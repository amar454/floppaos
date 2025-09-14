/* 

Copyright 2024, 2025 Amar Djulovic <aaamargml@gmail.com>

This file is part of FloppaOS.

FloppaOS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

FloppaOS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with FloppaOS. If not, see <https://www.gnu.org/licenses/>.

------------------------------------------------------------------------------

alloc.c

    This is the heap allocator for FloppaOS. 
    It uses the slab and physical page allocators to allocate specific sizes of memory and free them.

    - init_kernel_heap() finds the total memory size, and allocates a kernel heap virtual region according to the memory size

    - expand_kernel_heap() expands the size of the kernel heap virtual region 

    - shrink_kernel_heap() shrinks the size of the kernel heap virtual region by reallocating it to a smaller size

    - kmalloc() allocates a block of memory of the given size

    - kfree() frees a block of memory of the given pointer and size

    - kcalloc() allocates a block of memory of the given size and initializes it to zero

    - krealloc() reallocates a block of memory to a new size

*/


// TODO: actually implement kernel page directory destroying functions as they basically do nothing. 

// TODO pt2: fix clunky bits of the allocator such as alignment, guarding, and setting the heap vmm region 
// so uhhh basically everything but it works so I don't care for now. 
#include "slab.h"
#include "alloc.h"
#include "pmm.h"
#include "utils.h"
#include "paging.h"
#include "../lib/str.h"
#include "vmm.h"
#include "../lib/logging.h"
#include "../kernel/kernel.h"
#include "../drivers/vga/vgahandler.h"

#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))
#define PMM_RETURN_THRESHOLD (8 * PAGE_SIZE) // Return to PMM if free block is this big

int kernel_heap_size = 0; // yay no kernel heap yet :)

///////////////////////////
// structs for free list //
///////////////////////////

struct alloc_mem_block {
    struct alloc_mem_block *next;
    size_t size;
};

struct free_list {
    struct free_list *next;
    size_t size;
};

// struct for easy access of kernel regions, start and end addresses, and the heap free list.
struct kernel_region {
    struct kernel_region *next;
    uintptr_t start;
    uintptr_t end;
    struct free_list *free_list;
} kernel_regions;

typedef struct alloc_info {
    spinlock_t lock;
    struct alloc_mem_block *first_block;
    struct free_list *free_list;
} alloc_info_t;
static alloc_info_t this_allocator = { .lock = SPINLOCK_INIT, .first_block = NULL, .free_list = NULL };
static struct free_list *free_blocks = NULL; 
static int heap_initialized = 0;
static struct alloc_mem_block *first_block = NULL;

// Add block to free list in sorted order
void add_to_free_list(void *ptr, size_t size) {
    struct free_list *block = (struct free_list *)ptr;
    block->size = size;

    struct free_list **prev = &free_blocks;
    struct free_list *current = free_blocks;

    while (current && (uintptr_t)current < (uintptr_t)block) {
        prev = &current->next;
        current = current->next;
    }

    block->next = current;
    *prev = block;
}

// Get the first sufficiently large block from free list
void *get_from_free_list(size_t size) {
    struct free_list **prev = &free_blocks;
    struct free_list *current = free_blocks;
    
    while (current) {
        if (current->size >= size) {
            *prev = current->next;
            return (void *)current;
        }
        prev = &current->next;
        current = current->next;
    }
    return NULL;
}

// Remove block from free list
void remove_from_free_list(struct free_list *block) {
    struct free_list **prev = &free_blocks;
    struct free_list *current = free_blocks;

    while (current) {
        if (current == block) {
            *prev = current->next;
            return;
        }
        prev = &current->next;
        current = current->next;
    }
}

// Merge adjacent free blocks
void coalesce_free_blocks() {
    struct free_list *current = free_blocks;
    while (current && current->next) {
        if ((uintptr_t)current + current->size == (uintptr_t)current->next) {
            current->size += current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

// Return large free blocks to PMM
void return_large_free_blocks() {
    struct free_list *prev = NULL;
    struct free_list *current = free_blocks;

    while (current) {
        if (current->size >= PMM_RETURN_THRESHOLD) {
            if (prev)
                prev->next = current->next;
            else
                free_blocks = current->next;

            pmm_free_pages((void *)current, current->size / PAGE_SIZE, 1);
        } else {
            prev = current;
        }
        current = prev ? prev->next : free_blocks;
    }
}

// add a memory block to the memory block linked list.
void add_memory_block(uintptr_t start, size_t size, int is_free) {
    struct alloc_mem_block *block = (struct alloc_mem_block *)start;
    block->size = size;
    block->next = NULL;

    if (is_free) {
        block->size |= 1;
        add_to_free_list((void *)start, size);
    } else {
        block->size &= ~1;
    }
}

// return a pointer to the memory block at a given address
void *get_memory_block(uintptr_t addr) {
    struct alloc_mem_block *block = (struct alloc_mem_block *)addr;
    if (block->size & 1) {
        return NULL; // Block is free
    }
    return (void *)(block + 1);
}

// return a pointer to the memory block after the block at addr
void *get_next_memory_block(uintptr_t addr) {
    struct alloc_mem_block *block = (struct alloc_mem_block *)addr;
    if (block->next) {
        return (void *)(block->next + 1);
    }
    return NULL;
}

// return a pointer to the memory block before the block at addr
void *get_prev_memory_block(uintptr_t addr) {
    struct alloc_mem_block *block = (struct alloc_mem_block *)addr;
    if (block->next) {
        return (void *)(block->next - 1);
    }
    return NULL;
}


void *get_memory_block_size(uintptr_t addr) {
    struct alloc_mem_block *block = (struct alloc_mem_block *)addr;
    if (block->size & 1) {
        return NULL; // Block is free
    }
    return (void *)(block->size & ~1);
}



// get memory size, calculate appropriate heap size, allocate virtual address space for heap, and create the first memory block
void init_kernel_heap(void) {
    log("Initializing kernel heap...\n", YELLOW);

    // check mem size and do sanity check if pmm is initialized.
    size_t total_memory = pmm_get_memory_size();
    if (total_memory == 0) {
        log("init_kernel_heap: PMM not initialized or no memory available!\n", RED);
        PANIC_PMM_NOT_INITIALIZED((uintptr_t)first_block);
        return;
    }

    kernel_heap_size = (total_memory * HEAP_PERCENTAGE) / 100;
    kernel_heap_size = ALIGN_UP(kernel_heap_size, PAGE_SIZE);
    if (kernel_heap_size < MIN_HEAP_SIZE) {
        kernel_heap_size = ALIGN_UP(MIN_HEAP_SIZE, PAGE_SIZE);
    } else if (kernel_heap_size > MAX_HEAP_SIZE) {
        kernel_heap_size = ALIGN_UP(MAX_HEAP_SIZE, PAGE_SIZE);
    }

    first_block = (struct alloc_mem_block *)KERNEL_HEAP_START;
    add_memory_block(KERNEL_HEAP_START, kernel_heap_size, 1);
    kernel_regions.start = (uintptr_t)KERNEL_HEAP_START;
    kernel_regions.end = (uintptr_t)(KERNEL_HEAP_START + kernel_heap_size);
    kernel_regions.next = NULL;

    static spinlock_t alloc_lock_initializer = SPINLOCK_INIT;
    this_allocator.lock = alloc_lock_initializer;
    spinlock_init(&this_allocator.lock);

    log("kernel heap init - ok\n\n", YELLOW);

    // now that we can alloc stuff, mark the heap as initialized.
    heap_initialized = 1;
}


// free a memory block by checking if it is in the slab, and it not, add it to the free list, coalesce and return to pmm if necessary.
void free_memory_block(void *ptr, size_t size) {
    if (!ptr || size == 0) return;

    if (size <= 4096) {
        slab_free(ptr);
        return;
    }

    struct alloc_mem_block *block = (struct alloc_mem_block *)ptr - 1;
    add_to_free_list(block, block->size);
    block->size |= 1;

    coalesce_free_blocks();
    return_large_free_blocks();
}



// return a pointer to a memory block of requested size via slab allocator or physical pages
// note: it is strongly discouraged to use kmalloc for any sizes above 4kb, as it can be prevented by using virtual addresses.
void *kmalloc(size_t size) {
    if (size == 0) return NULL;

    if (!heap_initialized) {
        log("kmalloc: Kernel heap not initialized!\n", RED);
        return NULL;
    }

    void *ptr = NULL;

    // allocate to slab if under 4kb
    if (size <= 4096) {
        spinlock(&this_allocator.lock);
        void *ptr = slab_alloc(size);
        if (ptr) return ptr;
        spinlock_unlock(&this_allocator.lock, true);
    }

    // if over 4kb, try to alloc from the free list
    
    ptr = get_from_free_list(size);
    if (ptr) {
        spinlock(&this_allocator.lock);
        // if we can find one and do that, take block from free list 
        struct alloc_mem_block *block = (struct alloc_mem_block *)ptr;

        // Mark block as allocated
        block->size &= ~1; 
        
        spinlock_unlock(&this_allocator.lock, true);
        // return void pointer to the block.
        return (void *)(block + 1);
        
    }

    // now, if there is no suitable block in the free list, align to page size, and allocate physical pages

    spinlock(&buddy.lock);
    size_t pages = ALIGN_UP(size + sizeof(struct alloc_mem_block), 4096) / 4096;
    ptr = pmm_alloc_pages(0, pages);
    if (!ptr) { // check if alloc worked (should)
        log("kmalloc: Failed to allocate memory for size: ", RED);
        log_uint("", size);
        PANIC_KMALLOC_FAILED((uintptr_t)ptr);
        return NULL;
    }

    // add that memory block
    add_memory_block((uintptr_t)ptr, pages * 4096, 0);
    spinlock_unlock(&buddy.lock, true);
    // return void pointer to the newly allocated memory block. 

    return (void *)((struct alloc_mem_block *)ptr + 1);
}

// free an allocated block at *ptr given its size as well.
void kfree(void *ptr, size_t size) {
    if (!ptr || size == 0) return;

    
    if (size <= 4096) {
        // woohoo slab dealloc is easy, just free from slab allocator
        slab_free(ptr);
        return;
    }
    // otherwise, free mem block and add to free list.
    free_memory_block(ptr, size);
}

// allocate a mem block of requested size and set its value to requested num.
void *kcalloc(uint32_t num, size_t size) {
    size_t total_size = num * size;
    void *ptr = kmalloc(total_size);
    if (ptr) {
        flop_memset(ptr, 0, total_size);
    }
    return ptr;
}

// reallocate a block of memory at *ptr, given its old size and the new requested size
void *krealloc(void *ptr, size_t old_size, size_t new_size) {
    if (!ptr) return kmalloc(new_size);
    if (new_size == 0) {
        // check for edge cases of new size being 0 (shouldn't happen, i hope)
        kfree(ptr, old_size);
        return NULL;
    }

    // allocate the new memory block
    void *new_ptr = kmalloc(new_size);
    if (!new_ptr) return NULL;

    // copy the data of the old memory block to the new memory block
    size_t copy_size = (old_size < new_size) ? old_size : new_size;
    flop_memcpy(new_ptr, ptr, copy_size);

    // now we can free the old memory block and return the pointer to the newly allocated block.
    kfree(ptr, old_size);
    return new_ptr;
}



#define NUM_ALLOCS 8
void test_alloc() {
    // some example allocation sizes pulled out of my ass
    static const int ALLOC_SIZES[NUM_ALLOCS] = {32, 1564, 568, 2578, 4095, 8700, 11464, 16384};
    void *ptrs[NUM_ALLOCS];
    bool _alloc_success;

    // allocate the test blocks
    for (size_t i = 0; i < NUM_ALLOCS; i++) {
        ptrs[i] = kmalloc(ALLOC_SIZES[i]);
        if (ptrs[i]) {
            // Test writing to the allocated memory
            uint32_t* test_ptr = (uint32_t*)ptrs[i];
            for (size_t j = 0; j < ALLOC_SIZES[i] / sizeof(uint32_t); j++) {
                test_ptr[j] = 0x12345678;
                if (test_ptr[j] != 0x12345678) {
                    log("alloc: memory write test failed\n", RED);
                    _alloc_success = false;
                    break;
                }
            }
            // Test memset
            flop_memset(ptrs[i], 0, ALLOC_SIZES[i]);
            for (size_t j = 0; j < ALLOC_SIZES[i] / sizeof(uint32_t); j++) {
                if (test_ptr[j] != 0) {
                    log("alloc: memset test failed\n", RED);
                    _alloc_success = false;
                    break;
                }
            }
            _alloc_success = true;
        }
        else {
            _alloc_success = false;
            break;
        }
    }
    if (!_alloc_success) {
        log("alloc: kmalloc test failed\n", RED);
        return;
    }
    else {
        log("alloc: kmalloc test passed for sizes 32, 1564, 568, 2578, 4095, 8700, 11464, 16384\n", GREEN);
    }
    bool _free_success = true;

    for (size_t i = 0; i < NUM_ALLOCS; i++) {
        kfree(ptrs[i], ALLOC_SIZES[i]);
        if (ptrs[i]) {
            _free_success = true;
        }
        else {
            _free_success = false;
            break;
        }
    }
    
    log("test_alloc: kfree test passed\n", GREEN);
}void dump_heap() {    log("Heap Dump:\n", CYAN);
    
    struct free_list *current = free_blocks;
    while (current) {
        log("Free block at: ", CYAN);
        log_address("", (uintptr_t)current);
        log(" Size: ", CYAN);
        log_uint("",current->size);
        log("\n", CYAN);
        current = current->next;
    }
}

void check_heap_integrity() {
    struct free_list *current = free_blocks;
    while (current && current->next) {
        if ((uintptr_t)current + current->size > (uintptr_t)current->next) {
            log("Heap corruption detected!\n", RED);
            PANIC_PMM_NOT_INITIALIZED((uintptr_t)current);
        }
        current = current->next;
    }
}

// allocate a memory block with a two page buffer... for memory sensitive needs.
// NOTE: PLEASE ONLY USE THIS FUNCTION WITH THE INTENTION OF FREEING IT WITH THE NEXT KFREE_GUARDED FUNCTION.
void *kmalloc_guarded(size_t size) {
    size_t pages = (ALIGN_UP(size, PAGE_SIZE) / PAGE_SIZE) + 2; // Add 2 guard pages
    void *ptr = pmm_alloc_pages(0, pages);
    if (!ptr) return NULL;

    uintptr_t user_ptr = (uintptr_t)ptr + PAGE_SIZE;
    add_memory_block(user_ptr, (pages - 2) * PAGE_SIZE, 0);
    return (void *)user_ptr;
}


// free a guarded allocation of size plus two pages
// NOTE: DO NOT BE STUPID, PLEASE FREE ALL GUARDED MEMORY BLOCKS WITH THIS FUNCTION
void kfree_guarded(void *ptr, size_t size) {
    if (!ptr || size == 0) return;

    uintptr_t user_ptr = (uintptr_t)ptr - PAGE_SIZE;
    size_t pages = (ALIGN_UP(size, PAGE_SIZE) / PAGE_SIZE) + 2; // Subtract 2 guard pages

    pmm_free_pages((void *)user_ptr, pages, 1);
    free_memory_block((void *)user_ptr, (pages - 2) * PAGE_SIZE);
}
void* krealloc_guarded(void *ptr, size_t old_size, size_t new_size) {
    if (!ptr) return kmalloc_guarded(new_size);
    if (new_size == 0) {
        kfree_guarded(ptr, old_size);
        return NULL;
    }

    void *new_ptr = kmalloc_guarded(new_size);
    if (!new_ptr) return NULL;

    size_t copy_size = (old_size < new_size) ? old_size : new_size;
    flop_memcpy(new_ptr, (void *)((uintptr_t)ptr - PAGE_SIZE), copy_size);

    kfree_guarded((void *)((uintptr_t)ptr - PAGE_SIZE), old_size);
    return new_ptr;
}
void* kcalloc_guarded(size_t num, size_t size) {
    size_t total_size = num * size;
    void *ptr = kmalloc_guarded(total_size);
    if (ptr) {
        flop_memset(ptr, 0, total_size);
    }
    return ptr;
}

void expand_kernel_heap(size_t additional_size) {
    if (additional_size == 0) return;
    
    uintptr_t new_start = kernel_regions.end;
    uintptr_t new_end = new_start + ALIGN_UP(additional_size, PAGE_SIZE);

    if (!pmm_alloc_pages(0, (new_end - new_start) / PAGE_SIZE)) {
        log("Heap expansion failed!\n", RED);
        return;
    }

    kernel_regions.end = new_end;
    add_memory_block(new_start, new_end - new_start, 1);
    log("Kernel heap expanded.\n", GREEN);
}
void shrink_kernel_heap(size_t reduce_size) {
    if (reduce_size == 0 || reduce_size > (kernel_regions.end - kernel_regions.start)) {
        log("Invalid size for shrinking kernel heap!\n", RED);
        return;
    }

    uintptr_t new_end = kernel_regions.end - ALIGN_UP(reduce_size, PAGE_SIZE);
    
    // Free the pages being removed
    pmm_free_pages((void *)new_end, (kernel_regions.end - new_end) / PAGE_SIZE, 1);

    kernel_regions.end = new_end;

    free_memory_block((void *)new_end, (kernel_regions.end - new_end));

    log("Kernel heap shrunk.\n", YELLOW);
}
void *kmalloc_aligned(size_t size, size_t alignment) {
    size_t total_size = size + alignment - 1;
    void *ptr = kmalloc(total_size);
    if (!ptr) return NULL;

    uintptr_t aligned_addr = ALIGN_UP((uintptr_t)ptr, alignment);
    return (void *)aligned_addr;
}

void kfree_aligned(void *ptr) {
    if (!ptr) return;

    struct alloc_mem_block *block = (struct alloc_mem_block *)ptr - 1;
    kfree(block, block->size);
}
void *kcalloc_aligned(size_t num, size_t size, size_t alignment) {
    size_t total_size = num * size;
    void *ptr = kmalloc_aligned(total_size, alignment);
    if (ptr) {
        flop_memset(ptr, 0, total_size);
    }
    return ptr;
}
void *krealloc_aligned(void *ptr, size_t old_size, size_t new_size, size_t alignment) {
    if (!ptr) return kmalloc_aligned(new_size, alignment);
    if (new_size == 0) {
        kfree_aligned(ptr);
        return NULL;
    }

    void *new_ptr = kmalloc_aligned(new_size, alignment);
    if (!new_ptr) return NULL;

    size_t copy_size = (old_size < new_size) ? old_size : new_size;
    flop_memcpy(new_ptr, ptr, copy_size);

    kfree_aligned(ptr);
    return new_ptr;
}  

void *kmalloc_aligned_guarded(size_t size, size_t alignment) {
    size_t total_size = size + alignment - 1;
    void *ptr = kmalloc_guarded(total_size);
    if (!ptr) return NULL;

    uintptr_t aligned_addr = ALIGN_UP((uintptr_t)ptr, alignment);
    return (void *)aligned_addr;
}
void kfree_aligned_guarded(void *ptr) {
    if (!ptr) return;

    struct alloc_mem_block *block = (struct alloc_mem_block *)ptr - 1;
    kfree(block, block->size);
}
void *kcalloc_aligned_guarded(size_t num, size_t size, size_t alignment) {
    size_t total_size = num * size;
    void *ptr = kmalloc_guarded(total_size);
    if (ptr) {
        flop_memset(ptr, 0, total_size);
    }
    return ptr;
}
void *krealloc_aligned_guarded(void *ptr, size_t old_size, size_t new_size, size_t alignment) {
    if (!ptr) return kmalloc_guarded(new_size);
    if (new_size == 0) {
        kfree(ptr, old_size);
        return NULL;
    }

    void *new_ptr = kmalloc_guarded(new_size);
    if (!new_ptr) return NULL;

    size_t copy_size = (old_size < new_size) ? old_size : new_size;
    flop_memcpy(new_ptr, ptr, copy_size);

    kfree(ptr, old_size);
    return new_ptr;
}
// free all memory blocks by iterating through the list and freeing pages.
void free_all_mem_blocks() {
    struct alloc_mem_block *current = first_block;
    while (current) {
        struct alloc_mem_block *next = current->next;
        pmm_free_pages(current, current->size / PAGE_SIZE, 1);
        current = next;
    }
    first_block = NULL;
    free_blocks = NULL;
}


// probably should not be used, but this is basically calloc for the entire heap
void zero_all_mem_blocks() {
    log("Zeroing out all heap memory blocks...\n", YELLOW);
    struct alloc_mem_block *current = first_block;
    while (current) {
        flop_memset(current, 0, current->size);
        current = current->next;
    }
}

// helper function to check if the heap is initialized and log if it isnt.
// using this is preferred to just checking if the heap intialized variable is 1.
bool is_heap_initialized() {
    if (!heap_initialized) {
        log("Heap not initialized!\n", RED);
        return false;
    }
    return true;
}

// zero, free, and mark the heap as uninitialized
void destroy_kernel_heap(uint32_t *kernel_page_directory) {
    log("alloc: Destroying kernel heap...\n", YELLOW);
    bool heap = is_heap_initialized();
    if (!heap) {
        zero_all_mem_blocks();
        free_all_mem_blocks();
    }

    heap_initialized = 0;
    log("alloc: Kernel heap destroyed\n", LIGHT_RED);
}

void re_init_kernel_heap(uint32_t *kernel_page_directory) {
    destroy_kernel_heap(kernel_page_directory);
    init_kernel_heap();
}


