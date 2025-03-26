/* 

Copyright 2024-25 Amar Djulovic <aaamargml@gmail.com>

This file is part of FloppaOS.

FloppaOS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

FloppaOS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with FloppaOS. If not, see <https://www.gnu.org/licenses/>.

------------------------------------------------------------------------------

slab.c:

    This is the slab allocator for floppaOS.
    The slab allocator uses a cache of "slabs" to allocate memory.
    Each slab is a fixed-size block of memory that can be allocated and freed.
    The cache is divided into multiple "slab caches" based on the size of the objects in the cache.
    Each slab cache has a fixed number of slabs, and each slab has a fixed size.
    When a slab is freed, it's merged with its neighboring slabs to create larger slabs.
    This process continues until the slab is the size of the cache.

    slab_init(...) initializes the slab allocator.

    slab_alloc(...) allocates memory from the slab allocator.

    slab_free(...) frees memory allocated by the slab allocator.

------------------------------------------------------------------------------
*/
#include "slab.h"
#include "paging.h"
#include "pmm.h"
#include "../lib/logging.h"
#include "../drivers/vga/vgahandler.h"
#include "utils.h"
#include "vmm.h"
#include "../kernel.h"
#include <stdbool.h>
#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))

static slab_cache_t *slab_caches[SLAB_ORDER_COUNT];

/**
 * @name get_order
 * @author Amar Djulovic <aaamargml@gmail.com> 
 *
 * @brief Calculates the order for a given size
 * @param size Size to calculate order for
 * @return Size order
 */
static size_t get_order(size_t size) {
    if (size == 0) return 0;
    size_t order = 0;
    size_t size_aligned = ALIGN_UP(size, SLAB_PAGE_SIZE);
    while (size_aligned > SLAB_PAGE_SIZE) {
        size_aligned >>= 1;
        order++;
    }
    return order;
}

/**
 * @name create_slab_cache
 * @author Amar Djulovic <aaamargml@gmail.com> 
 *
 * @brief Creates a new slab cache
 * @param size Size of objects in cache
 * @return Pointer to created cache or NULL if failed
 */
static slab_cache_t *create_slab_cache(size_t size) {
    if (size == 0) return NULL;
    size_t order = get_order(size);

    slab_cache_t *cache = (slab_cache_t *)pmm_alloc_pages(0);
    if (!cache) return NULL;

    cache->object_size = size;
    cache->num_objects = ((SLAB_PAGE_SIZE * (1 << order)) / size);
    if (cache->num_objects == 0) {
        pmm_free_pages(cache, 0);
        return NULL;
    }
    cache->free_count = 0;
    cache->free_list = NULL;
    cache->slab_list = NULL;

    return cache;
}

/**
 * @name create_slab
 * @author Amar Djulovic <aaamargml@gmail.com> 
 *
 * @brief Creates a new slab
 * @param cache Cache to create slab for
 * @param order Size order of slab
 * @return Pointer to created slab or NULL if failed
 */
static slab_t *create_slab(slab_cache_t *cache, size_t order) {
    if (!cache) {
        log_step("slab: Invalid cache!\n", RED);
        PANIC_INVALID_SLAB_CACHE((uintptr_t)cache);
        return NULL;
    }
    slab_t *slab = (slab_t *)pmm_alloc_pages(order);
    if (!slab) {
        log_step("slab: Failed to allocate slab!\n", RED);
        PANIC_FAILED_TO_CREATE_SLAB((uintptr_t)slab);
        return NULL;
    }

    slab->num_objects = cache->num_objects;
    slab->free_count = slab->num_objects;
    slab->next = cache->slab_list;
    cache->slab_list = slab;

    size_t available_space = (SLAB_PAGE_SIZE * (1 << order)) - sizeof(slab_t);
    if (available_space < cache->object_size * slab->num_objects) {
        pmm_free_pages(slab, order);
        return NULL;
    }

    uint8_t *ptr = (uint8_t *)(slab + 1);
    for (size_t i = 0; i < slab->num_objects; i++) {
        *(uint8_t **)ptr = cache->free_list;
        cache->free_list = ptr;
        ptr += cache->object_size;
    }
    cache->free_count = slab->num_objects;

    return slab;
}

/**
 * @name slab_init
 * @author Amar Djulovic <aaamargml@gmail.com> 
 *
 * @brief Initializes the slab allocator
 */
void slab_init(void) {
    log_step("Initializing slab allocator", LIGHT_GRAY);

    for (size_t i = 0; i < SLAB_ORDER_COUNT; i++) {
        size_t size = SLAB_MIN_SIZE << i;
        if (size == 0) continue;
        slab_caches[i] = create_slab_cache(size);
        if (slab_caches[i]) {
            log_step("Initialized slab cache for size: ", GREEN);
            log_uint("", size);
        }
    }
}

/**
 * @name slab_alloc
 * @author Amar Djulovic <aaamargml@gmail.com> 
 *
 * @brief Allocates memory from slab allocator
 * @param size Size to allocate
 * @return Pointer to allocated memory or NULL if failed
 */
void *slab_alloc(size_t size) {
    if (size == 0 || size > SLAB_MAX_SIZE) return NULL;

    size_t order = get_order(size);
    if (order >= SLAB_ORDER_COUNT) return NULL;

    slab_cache_t *cache = slab_caches[order];
    if (!cache) {
        log_step("slab: Invalid cache!\n", RED);
        PANIC_INVALID_SLAB_CACHE((uintptr_t)cache);
        return NULL;
    }

    if (cache->free_count > 0 && cache->free_list) {
        void *ptr = cache->free_list;
        cache->free_list = *(uint8_t **)ptr;
        cache->free_count--;
        return ptr;
    }

    if (!create_slab(cache, order)) {
        log_step("slab: Failed to create slab!\n", RED);
        PANIC_FAILED_TO_CREATE_SLAB((uintptr_t)cache->free_list); 
        return NULL;
    }
    if (cache->free_count == 0 || !cache->free_list) {
        log_step("slab: Failed to allocate memory from slab!\n", RED);
        PANIC_FAILED_SLAB_ALLOCATION((uintptr_t)cache->free_list);
        return NULL;
    }
    void *ptr = cache->free_list;
    cache->free_list = *(uint8_t **)ptr;
    cache->free_count--;
    return ptr;
}

/**
 * @name remove_slab_from_cache
 * @author Amar Djulovic <aaamargml@gmail.com> 
 *
 * @brief Removes a slab from its cache
 * @param cache Cache containing the slab
 * @param slab Slab to remove
 */
static void remove_slab_from_cache(slab_cache_t *cache, slab_t *slab) {
    if (slab == cache->slab_list) {
        cache->slab_list = slab->next;
    } else {
        slab_t *prev = cache->slab_list;
        while (prev && prev->next != slab) {
            prev = prev->next;
        }
        if (prev) {
            prev->next = slab->next;
        }
    }
}

/**
 * @name find_containing_cache
 * @author Amar Djulovic <aaamargml@gmail.com> 
 *
 * @brief Finds cache containing a slab
 * @param containing_slab Slab to find cache for
 * @param out_order Order of found cache
 * @return Pointer to found cache or NULL if not found
 */
static slab_cache_t* find_containing_cache(slab_t *containing_slab, size_t *out_order) {
    size_t order = 0;
    
    while (order < SLAB_ORDER_COUNT) {
        slab_cache_t *cache = slab_caches[order];
        if (!cache) {
            order++;
            continue;
        }

        slab_t *slab = cache->slab_list;
        while (slab) {
            if (slab == containing_slab) {
                *out_order = order;
                return cache;
            }
            slab = slab->next;
        }
        order++;
    }
    log_step("slab: Failed to find cache containing slab!\n", RED);
    PANIC_FAILED_TO_FIND_CACHE_CONTAINING_SLAB((uintptr_t)containing_slab);
    return NULL;
}

/**
 * @name add_to_free_list
 * @author Amar Djulovic <aaamargml@gmail.com> 
 *
 * @brief Adds memory to free list
 * @param cache Cache to add to
 * @param ptr Memory to add
 */
static void add_to_free_list(slab_cache_t *cache, void *ptr) {
    *(uint8_t **)ptr = cache->free_list;
    cache->free_list = ptr;
    cache->free_count++;
}

/**
 * @name slab_free
 * @author Amar Djulovic <aaamargml@gmail.com> 
 *
 * @brief Frees memory allocated by slab allocator
 * @param ptr Memory to free
 */
void slab_free(void *ptr) {
    if (!ptr) return;

    uintptr_t ptr_addr = (uintptr_t)ptr;
    uintptr_t page_addr = ptr_addr & ~(SLAB_PAGE_SIZE - 1);
    slab_t *containing_slab = (slab_t *)page_addr;

    if (ptr_addr < page_addr + sizeof(slab_t)) return;

    size_t order;
    slab_cache_t *cache = find_containing_cache(containing_slab, &order);
    if (!cache) {
        PANIC_SLAB_CACHE_NOT_FOUND((uintptr_t)cache);
        return;
    }
    add_to_free_list(cache, ptr);
    containing_slab->free_count++;

    if (containing_slab->free_count == containing_slab->num_objects) {
        remove_slab_from_cache(cache, containing_slab);
        pmm_free_pages(containing_slab, order);
    }
}

/**
 * @name slab_debug
 * @author Amar Djulovic <aaamargml@gmail.com> 
 *
 * @brief Prints debug information about slab allocator
 */
void slab_debug(void) {
    // print all caches, slabs, and their free lists
    for (size_t i = 0; i < SLAB_ORDER_COUNT; i++) {
        slab_cache_t *cache = slab_caches[i];
        if (!cache) continue; 
        log_f("Slab cache size:", cache->object_size);
        log_f("  Free count:", cache->free_count);

        slab_t *slab = cache->slab_list;
        while (slab) {
            log_f("    Slab free count:", slab->free_count);
            slab = slab->next;
        }
    }
}

/**
 * @name slab_realloc
 * @author Amar Djulovic <aaamargml@gmail.com> 
 *
 * @brief Destroys a slab
 * @param slab Slab to destroy
 * @param order Order of slab
 */
static void destroy_slab(slab_t *slab, size_t order) {
    pmm_free_pages(slab, order);
}

/**
 * @brief Checks if slab is empty
 * @param slab Slab to check
 * @return true if empty, false otherwise
 */
static bool is_slab_empty(slab_t *slab) {
    return slab->free_count == slab->num_objects;
}

/**
 * @name slab_realloc
 * @author Amar Djulovic <aaamargml@gmail.com> 
 *
 * @brief Validates pointer
 * @param ptr Pointer to validate
 * @param page_addr Page address
 * @return true if valid, false otherwise
 */
static bool is_ptr_valid(void *ptr, uintptr_t page_addr) {
    uintptr_t ptr_addr = (uintptr_t)ptr;
    return ptr && ptr_addr >= page_addr + sizeof(slab_t);
}

/**
 * @name slab_realloc
 * @author Amar Djulovic <aaamargml@gmail.com>
 *
 * @brief Gets slab containing pointer
 * @param ptr Pointer to find slab for
 * @return Pointer to containing slab
 */
static slab_t* get_containing_slab(void *ptr) {
    uintptr_t ptr_addr = (uintptr_t)ptr;
    return (slab_t *)(ptr_addr & ~(SLAB_PAGE_SIZE - 1));
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

/**
 * @name slab_realloc
 * @author Amar Djulovic <aaamargml@gmail.com>
 *
 * @brief Reallocates memory
 * @param ptr Pointer to reallocate
 * @param new_size New size
 * @return Pointer to reallocated memory or NULL if failed
 */
void* slab_realloc(void *ptr, size_t new_size) {
    if (!ptr) return slab_alloc(new_size);
    if (!new_size) {
        slab_free(ptr);
        return NULL;
    }

    void *new_ptr = slab_alloc(new_size);
    if (!new_ptr) return NULL;

    slab_t *old_slab = get_containing_slab(ptr);
    size_t old_order;
    slab_cache_t *old_cache = find_containing_cache(old_slab, &old_order);
    if (!old_cache) return new_ptr;

    // copy from ptr to new_ptr, and free new_ptr    
    flop_memcpy(new_ptr, ptr, MIN(new_size, old_cache->object_size));
    slab_free(ptr);
    return new_ptr;
}

/**
 * @name slab_realloc
 * @author Amar Djulovic <aaamargml@gmail.com> 
 *
 * @brief Allocates zeroed memory
 * @param num Number of elements
 * @param size Size of each element
 * @return Pointer to allocated memory or NULL if failed
 */
void* slab_calloc(size_t num, size_t size) {
    size_t total = num * size;
    void *ptr = slab_alloc(total);
    // set ptr=0 
    if (ptr) flop_memset(ptr, 0, total);
    return ptr;
}

/**
 * @brief Allocates aligned memory
 * @param alignment Alignment requirement
 * @param size Size to allocate
 * @return Pointer to allocated memory or NULL if failed
 */
void* slab_aligned_alloc(size_t alignment, size_t size) {
    if (!alignment || (alignment & (alignment - 1))) return NULL;
    size_t padded_size = size + alignment - 1;
    void *ptr = slab_alloc(padded_size);
    if (!ptr) return NULL;
    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t aligned = (addr + alignment - 1) & ~(alignment - 1);
    return (void*)aligned;
}

/**
 * @brief Gets allocated size of memory
 * @param ptr Pointer to memory
 * @return Size of allocation or 0 if invalid
 */
size_t slab_get_allocated_size(void *ptr) {
    if (!ptr) return 0;
    slab_t *slab = get_containing_slab(ptr);
    size_t order;
    slab_cache_t *cache = find_containing_cache(slab, &order);
    return cache ? cache->object_size : 0;
}

/**
 * @brief Resizes allocated memory
 * @param ptr Pointer to resize
 * @param new_size New size
 * @return Pointer to resized memory or NULL if failed
 */
void *slab_resize(void *ptr, size_t new_size) {
    if (!ptr) return slab_alloc(new_size);
    if (!new_size) {
        slab_free(ptr);
        return NULL;
    }
    
    // get the slab where ptr is stored
    slab_t *slab = get_containing_slab(ptr);
    size_t order;
    slab_cache_t *cache = find_containing_cache(slab, &order);
    if (!cache) return NULL;
    
    // alloc new slab size and copy old slab to it
    void *new_ptr = slab_alloc(new_size);
    flop_memcpy(new_ptr, ptr, cache->object_size);
    // free old slab
    slab_free(ptr);
    return new_ptr;     
}

