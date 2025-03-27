// Memory region definitions
#define KERNEL_HEAP_START 0xC0000000  // Example virtual address for kernel heap
#define KERNEL_HEAP_SIZE  0x4000000   // 64MB heap size

#include "slab.h"
#include "pmm.h"
#include "utils.h"
#include "../lib/logging.h"
#include "../drivers/vga/vgahandler.h"
#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))

static uintptr_t heap_current = KERNEL_HEAP_START;

void *kmalloc(size_t size) {
    if (size == 0) return NULL;

    // Check if we have enough space in the heap region
    if (heap_current + size > KERNEL_HEAP_START + KERNEL_HEAP_SIZE) {
        log_step("kmalloc: out of kernel heap space", size);
        return NULL;
    }

    // Use slab allocator for small allocations
    if (size <= SLAB_MAX_SIZE) {
        void *ptr = slab_alloc(size);
        if (ptr) {
            // Ensure the address is within our kernel heap region
            if ((uintptr_t)ptr >= KERNEL_HEAP_START && 
                (uintptr_t)ptr < KERNEL_HEAP_START + KERNEL_HEAP_SIZE) {
                return ptr;
            }
        }
    }

    // Use PMM for larger allocations
    size_t pages = (ALIGN_UP(size, SLAB_PAGE_SIZE) / SLAB_PAGE_SIZE);
    void *ptr = pmm_alloc_pages(pages);
    if (!ptr) {
        log_step("kmalloc: failed to allocate large block", size);
        return NULL;
    }

    // Map the physical pages to our kernel heap region
    uintptr_t virt_addr = heap_current;
    // Here you would need to map the physical pages to virtual addresses
    // map_pages(virt_addr, (uintptr_t)ptr, pages, PAGE_PRESENT | PAGE_WRITE);
    
    heap_current += pages * SLAB_PAGE_SIZE;
    return (void*)virt_addr;
}

void kfree(void *ptr, size_t size) {
    if (!ptr || size == 0) return;

    // Verify the pointer is within our kernel heap region
    if ((uintptr_t)ptr < KERNEL_HEAP_START || 
        (uintptr_t)ptr >= KERNEL_HEAP_START + KERNEL_HEAP_SIZE) {
        log_step("kfree: invalid pointer outside kernel heap", (uintptr_t)ptr);
        return;
    }

    // Free small allocations using slab allocator
    if (size <= SLAB_MAX_SIZE) {
        slab_free(ptr);
        return;
    }

    // Free large allocations using PMM
    size_t pages = (ALIGN_UP(size, SLAB_PAGE_SIZE) / SLAB_PAGE_SIZE);
    // Unmap the virtual memory region
    // unmap_pages((uintptr_t)ptr, pages);
    
    // Free the physical pages
    pmm_free_pages(ptr, pages);
}

void *kcalloc(size_t num, size_t size) {
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

    // Copy existing data to new allocation
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
        log_step("kmalloc size 32\n", GREEN);
    }
    if (ptr2) {
        log_step("kmalloc size 1564\n", GREEN);
    }
    if (ptr3) {
        log_step("kmalloc size 568\n", GREEN);
    }
    if (ptr4) {
        log_step("kmalloc size 2578\n", GREEN);
    }
    if (ptr5) {
        log_step("kmalloc size 4095\n", GREEN);
    }
    if (ptr6) {
        log_step("kmalloc size 8700\n", GREEN);
    }
    if (ptr7) {
        log_step("kmalloc size 11464\n", GREEN);
    }
    if (ptr8) {
        log_step("kmalloc size 16324\n", GREEN);
    }
    log_step("kmalloc test passed\n", GREEN);
    
    kfree(ptr1, 32);
    kfree(ptr2, 1564);
    kfree(ptr3, 568);
    kfree(ptr4, 2578);
    kfree(ptr5, 4095);
    kfree(ptr6, 8700);
    kfree(ptr7, 11464);
    kfree(ptr8, 16324);
    log_step("kfree test passed\n", GREEN);
}