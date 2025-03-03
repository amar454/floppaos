
#include "pmm.h"
#include "alloc.h"
#include "utils.h"
#include <stdint.h> 

// forward declarations

void* malloc(uint32_t size);
void free(void* ptr);
void* realloc(void* ptr, uint32_t size);
void* calloc(uint32_t num, uint32_t size);


/**
 * @name malloc
 * @author Amar Djulovic <aaamargml@gmail.com>
 *
 * @brief Allocate a block of memory
 *
 * @param size The size of the block to allocate
 * @return void* 
 * @returns A pointer to the allocated block of memory
 *
 * @note This function allocates a block of memory of the specified size, closest to the order of the size
 */
void* malloc(uint32_t size) {
    if (size == 0) return NULL;

    uint32_t total_size = size + sizeof(struct Page);
    uint32_t pages_needed = (total_size + PAGE_SIZE - 1) / PAGE_SIZE;  // Round up to full pages

    void* first_addr = NULL;  // Store the first allocated block
    uint32_t allocated_pages = 0;

    // Start from the largest possible order and allocate downwards
    for (int order = 31; order >= 0 && allocated_pages < pages_needed; order--) {
        uint32_t order_size = (1 << order) / PAGE_SIZE; // Convert order to pages

        while (allocated_pages + order_size <= pages_needed) {
            void* addr = pmm_alloc_pages(order);
            if (!addr) break;  // Stop if allocation fails for this order

            if (!first_addr) first_addr = addr;  // Store the first allocated block
            allocated_pages += order_size;
        }
    }

    if (allocated_pages < pages_needed) {
        // Failed to allocate everything, should implement cleanup logic here
        return NULL;
    }

    return first_addr;
}


/**
 * @name calloc
 * @author Amar Djulovic <aaamargml@gmail.com>
 * 
 * @brief Allocate and clear memory
 *
 * @param num 
 * @param size 
 * @return void* 
 * @returns A pointer to the allocated block of memory
 *
 * @note This will allocate memory and set it to zero, then return a pointer to the memory.
 */
void* calloc(uint32_t num, uint32_t size) {
    uint32_t total_size = num * size;
    void* addr = malloc(total_size);
    if (addr) {
        flop_memset(addr, 0, total_size);
    }
    return addr;
}

/**
 * @name realloc
 * @author Amar Djulovic <aaamargml@gmail.com>
 *
 * @brief Reallocate memory
 * 
 * @param ptr 
 * @param size 
 * @return void* 
 * @returns A pointer to the allocated block of memory
 *
 * @note This function reallocates memory, copying the old data to the new block
 */
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
/**
 * @name free
 * @author Amar Djulovic <aaamargml@gmail.com>

 * @brief Free memory
 * 
 * @param ptr 
 * @return void
 *
 * @note This function frees memory, and sets the pointer to NULL
 */
void free(void* ptr) {
    if (!ptr) return;
    pmm_free_pages(ptr, 0);
}
