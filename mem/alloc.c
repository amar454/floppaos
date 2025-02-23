#include "pmm.h"
#include "alloc.h"
#include "utils.h"
#include <stdint.h> 
/**
 * @name malloc

 * @brief Allocate a block of memory

 * @param size The size of the block to allocate
 * @return A pointer to the allocated block, or NULL if the allocation failed

 
 */
void* malloc(uint32_t size) {
    if (size == 0) return NULL;
    
    uint32_t order = 0;
    uint32_t total_size = size + sizeof(struct Page);
    while ((1 << order) < total_size) {
        order++;
    }

    void* addr = pmm_alloc_pages(order);
    if (!addr) return NULL;
    return addr;
}

void* calloc(uint32_t num, uint32_t size) {
    uint32_t total_size = num * size;
    void* addr = malloc(total_size);
    if (addr) {
        flop_memset(addr, 0, total_size);
    }
    return addr;
}

void* realloc(void* ptr, uint32_t size) {
    if (!ptr) {
        return malloc(size);
    }

    if (size == 0) {
        free(ptr);
        return NULL;
    }

    void* new_ptr = malloc(size);
    if (new_ptr) {
        struct Page* page = (struct Page*)ptr;
        uint32_t old_size = (1 << page->order) * PAGE_SIZE - sizeof(struct Page);
        flop_memcpy(new_ptr, ptr, old_size < size ? old_size : size);
        free(ptr);
    }
    return new_ptr;
}

void free(void* ptr) {
    if (!ptr) return;
    pmm_free_pages(ptr, 0);
}