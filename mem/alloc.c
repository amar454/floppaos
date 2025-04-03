#include "slab.h"
#include "alloc.h"
#include "pmm.h"
#include "utils.h"
#include "../lib/logging.h"
#include "../kernel.h"
#include "../drivers/vga/vgahandler.h"

#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))
#define PMM_RETURN_THRESHOLD (8 * PAGE_SIZE) // Return to PMM if free block is this big

int kernel_heap_size = 0;

struct alloc_mem_block {
    struct alloc_mem_block *next;
    size_t size;
};

struct free_list {
    struct free_list *next;
    size_t size;
};

struct kernel_region {
    struct kernel_region *next;
    uintptr_t start;
    uintptr_t end;
    struct free_list *free_list;
} kernel_regions;

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

            pmm_free_pages((void *)current, current->size / PAGE_SIZE);
        } else {
            prev = current;
        }
        current = prev ? prev->next : free_blocks;
    }
}

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

void init_kernel_heap(void) {
    log_step("Initializing kernel heap...\n", YELLOW);
    
    size_t total_memory = pmm_get_memory_size();
    if (total_memory == 0) {
        log_step("init_kernel_heap: PMM not initialized or no memory available!\n", RED);
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

    log_step("Kernel heap initialized successfully\n", GREEN);
    heap_initialized = 1;
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;

    if (size <= SLAB_MAX_SIZE) {
        void *ptr = slab_alloc(size);
        if (ptr) return ptr;
    }

    void *ptr = get_from_free_list(size);
    if (ptr) {
        struct alloc_mem_block *block = (struct alloc_mem_block *)ptr;
        block->size &= ~1; // Mark as used
        return (void *)(block + 1);
    }

    size_t pages = (ALIGN_UP(size + sizeof(struct alloc_mem_block), SLAB_PAGE_SIZE) / SLAB_PAGE_SIZE);
    ptr = pmm_alloc_pages(0, pages);
    if (!ptr) {
        log_step("kmalloc: failed to allocate large block", size);
        return NULL;
    }

    add_memory_block((uintptr_t)ptr, pages * SLAB_PAGE_SIZE, 0);
    return (void *)((struct alloc_mem_block *)ptr + 1);
}

void kfree(void *ptr, size_t size) {
    if (!ptr || size == 0) return;

    if (size <= SLAB_MAX_SIZE) {
        slab_free(ptr);
        return;
    }

    struct alloc_mem_block *block = (struct alloc_mem_block *)ptr - 1;
    add_to_free_list(block, block->size);
    block->size |= 1;

    coalesce_free_blocks();
    return_large_free_blocks();
}

void *kcalloc(uint32_t num, size_t size) {
    size_t total_size = num * size;
    void *ptr = kmalloc(total_size);
    if (ptr) {
        flop_memset(ptr, 0, total_size);
    }
    return ptr;
}

void *krealloc(void *ptr, size_t old_size, size_t new_size) {
    if (!ptr) return kmalloc(new_size);
    if (new_size == 0) {
        kfree(ptr, old_size);
        return NULL;
    }

    void *new_ptr = kmalloc(new_size);
    if (!new_ptr) return NULL;

    size_t copy_size = (old_size < new_size) ? old_size : new_size;
    flop_memcpy(new_ptr, ptr, copy_size);

    kfree(ptr, old_size);
    return new_ptr;
}


#define NUM_ALLOCS 8
#define BUFFER_SIZE 64
void test_alloc() {
    static const int ALLOC_SIZES[NUM_ALLOCS] = {32, 1564, 568, 2578, 4095, 8700, 11464, 16384};
    void *ptrs[NUM_ALLOCS];

    char *buffer = (char *)kmalloc(BUFFER_SIZE);
    if (!buffer) {
        log_step("Failed to allocate buffer for logging\n", RED);
        return;
    }

    for (size_t i = 0; i < NUM_ALLOCS; i++) {
        ptrs[i] = kmalloc(ALLOC_SIZES[i]);
        if (ptrs[i]) {
            flopsnprintf(buffer, BUFFER_SIZE, "kmalloc test passed for size %d\n", ALLOC_SIZES[i]);
            log_step(buffer, GREEN);
        }
    }


    for (size_t i = 0; i < NUM_ALLOCS; i++) {
        kfree(ptrs[i], ALLOC_SIZES[i]);
    }

    // big allocation
    static const int EXTRA_ALLOC_SIZE = 123454;
    void *ptr9 = kmalloc(EXTRA_ALLOC_SIZE);
    if (ptr9) {
        flopsnprintf(buffer, BUFFER_SIZE, "kmalloc test passed for size %d\n", EXTRA_ALLOC_SIZE);
        log_step(buffer, GREEN);
        kfree(ptr9, EXTRA_ALLOC_SIZE);
    }

    log_step("kfree test passed\n", GREEN);

    // Free print buffer after use
    kfree(buffer, BUFFER_SIZE);
}