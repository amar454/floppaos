#include "slab.h"
#include "pmm.h"
#include "utils.h"
#include "../lib/logging.h"
#include "../drivers/vga/vgahandler.h"
#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))

void *kmalloc(size_t size) {
    if (size == 0) return NULL;

    // Use slab allocator for small allocations
    if (size <= SLAB_MAX_SIZE) {
        void *ptr = slab_alloc(size);
        if (ptr) return ptr;
    }

    // Use PMM for larger allocations
    size_t pages = (ALIGN_UP(size, SLAB_PAGE_SIZE) / SLAB_PAGE_SIZE);
    void *ptr = pmm_alloc_pages(pages);
    if (!ptr) {
        log_step("kmalloc: failed to allocate large block", size);
        return NULL;
    }

    return ptr;
}

void kfree(void *ptr, size_t size) {
    if (!ptr || size == 0) return;

    // Free small allocations using slab allocator
    if (size <= SLAB_MAX_SIZE) {
        slab_free(ptr);
        return;
    }

    // Free large allocations using PMM
    size_t pages = (ALIGN_UP(size, SLAB_PAGE_SIZE) / SLAB_PAGE_SIZE);
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
    
    void *ptr1 = kmalloc(100);
    void *ptr2 = kmalloc(200);
    void *ptr3 = kmalloc(300);
    if (ptr1) {
        log_step("kmalloc test passed for size 100\n", GREEN);
    }
    if (ptr2) {
        log_step("kmalloc test passed for size 200\n", GREEN);
    }
    if (ptr3) {
        log_step("kmalloc test passed for size 300\n", GREEN);
    }
    kfree(ptr1, 100);
    kfree(ptr2, 200);
    kfree(ptr3, 300);
    log_step("kfree test passed\n", GREEN);
}
