#include "slab.h"
#include "alloc.h"
#include "pmm.h"
#include "utils.h"
#include "../lib/logging.h"
#include "../kernel.h"
#include "../drivers/vga/vgahandler.h"
#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))
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

void add_to_free_list(void *ptr, size_t size) {
    struct free_list *block = (struct free_list *)ptr;
    block->size = size;
    block->next = free_blocks;
    free_blocks = block;
}

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


static int heap_initialized = 0;
static struct alloc_mem_block *first_block = NULL;

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


    log_step("Kernel heap and region initialized successfully\n", GREEN);
    log_address("Heap start address: ", KERNEL_HEAP_START);
    log_address("Heap end address: ", KERNEL_HEAP_START + kernel_heap_size);
    log_uint("Heap size (KB): ", kernel_heap_size / 1024);
    log_uint("Heap size (MB): ", kernel_heap_size / (1024 * 1024));
    log_uint("Percentage of total memory: ", HEAP_PERCENTAGE);
    
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
    block->size |= 1; // Mark as free

    // Coalesce adjacent free blocks
    struct free_list *current = free_blocks;
    struct free_list *prev = NULL;
    
    while (current && current->next) {
        if ((uintptr_t)current + current->size == (uintptr_t)current->next) {
            current->size += current->next->size;
            current->next = current->next->next;
        } else {
            prev = current;
            current = current->next;
        }
    }
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

void test_alloc() {
    void *ptr1 = kmalloc(32);
    void *ptr2 = kmalloc(1564);
    void *ptr3 = kmalloc(568);
    void *ptr4 = kmalloc(2578);
    void *ptr5 = kmalloc(4095);
    void *ptr6 = kmalloc(8700);
    void *ptr7 = kmalloc(11464);
    void *ptr8 = kmalloc(16324);
    
    if (ptr1) {
        log_step("kmalloc test passed for size 32\n", GREEN);
    }
    if (ptr2) {
        log_step("kmalloc test passed for size 1564\n", GREEN);
    }
    if (ptr3) {
        log_step("kmalloc test passed for size 568\n", GREEN);
    }
    if (ptr4) {
        log_step("kmalloc test passed for size 2578\n", GREEN);
    }
    if (ptr5) {
        log_step("kmalloc test passed for size 4095\n", GREEN);
    }
    if (ptr6) {
        log_step("kmalloc test passed for size 8700\n", GREEN);
    }

    if (ptr7) {
        log_step("kmalloc test passed for size 11464\n", GREEN);
    }
    if (ptr8) {
        log_step("kmalloc test passed for size 16384\n", GREEN);
    }


    kfree(ptr1, 32);

    kfree(ptr2, 1564);
    kfree(ptr3, 512);
    kfree(ptr4, 2048);
    kfree(ptr5, 4096);
    kfree(ptr6, 8192);
    kfree(ptr7, 11464);



    void *ptr9 = kmalloc(123454);
    
    if (ptr9) {
        log_step("kmalloc test passed for size 123454\n", GREEN);
    }
    kfree(ptr9, 123454);


    log_step("kfree test passed\n", GREEN);
}
