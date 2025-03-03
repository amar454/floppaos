
#include "pmm.h"
#include "alloc.h"
#include "utils.h"
#include <stdint.h> 

// forward declarations

void* malloc(uint32_t size);
void free(void* ptr);
void* realloc(void* ptr, uint32_t size);
void* calloc(uint32_t num, uint32_t size);
struct AllocNode {
    void* addr;
    uint32_t order;
    struct AllocNode* next;
};

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
    uint32_t pages_needed = (total_size + PAGE_SIZE - 1) / PAGE_SIZE;  // Round up

    void* first_addr = NULL;
    uint32_t allocated_pages = 0;
    struct AllocNode* head = NULL;

    // Try allocating from the largest possible order down to the smallest
    for (int order = 31; order >= 0 && allocated_pages < pages_needed; order--) {
        uint32_t order_size = (1 << order) / PAGE_SIZE;  // Convert order to pages

        while (allocated_pages + order_size <= pages_needed) {
            void* addr = pmm_alloc_pages(order);
            if (!addr) break;  // Stop if allocation fails

            // Track allocation for cleanup in case of failure
            struct AllocNode* node = (struct AllocNode*)pmm_alloc_pages(0); // Single-page allocation for tracking
            if (!node) {
                // If we fail to allocate the tracking node, free already allocated memory
                while (head) {
                    struct AllocNode* temp = head;
                    pmm_free_pages(head->addr, head->order);
                    head = head->next;
                    pmm_free_pages(temp, 0); // Free tracking node
                }
                return NULL;
            }
            node->addr = addr;
            node->order = order;
            node->next = head;
            head = node;

            if (!first_addr) first_addr = addr;  // Store the first allocated block
            allocated_pages += order_size;
        }
    }

    // If we didn't allocate enough, clean up and return NULL
    if (allocated_pages < pages_needed) {
        while (head) {
            struct AllocNode* temp = head;
            pmm_free_pages(head->addr, head->order);
            head = head->next;
            pmm_free_pages(temp, 0); // Free tracking node
        }
        return NULL;
    }

    // Free tracking list since allocation was successful
    while (head) {
        struct AllocNode* temp = head;
        head = head->next;
        pmm_free_pages(temp, 0); // Free tracking node
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
