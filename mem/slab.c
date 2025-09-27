/* 

Copyright 2024, 2025 Amar Djulovic <aaamargml@gmail.com>

This file is part of FloppaOS.

FloppaOS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

FloppaOS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with FloppaOS. If not, see <https://www.gnu.org/licenses/>.

------------------------------------------------------------------------------

slab.c

    This is the slab allocator for floppaOS. 
    It prevents memory fragmentation for small allocations.
    It creates fixed caches of memory sizes (called objects), and in my implementation, those are powers of two.
    
    For example, lets say we call slab_alloc(50).
    The allocator will first
        1. look for an available obj of the closest size, and in this instance, the closest size to 50 is 64, so we will return a 64 byte object.
        2. If we cannot find a cache of the appropriate size, we will create a new cache, or a new slab.

    - slab_alloc() returns a ptr to an obj of the closest size of the amount requested `size`

    - slab_free() will find the slab at the addr specified `ptr`, and add it to the free list.

*/
#include "slab.h"
#include "pmm.h"
#include "../lib/logging.h"
#include "../kernel/kernel.h"
#include "../drivers/vga/vgahandler.h"
#include "utils.h"
#include <stdbool.h>

// macros for alignment.
#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

static slab_cache_t *slab_caches[SLAB_ORDER_COUNT];

// get order of given size
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

static bool initialize_slab(slab_t *slab, slab_cache_t *cache, size_t order) {
    log_f("Initializing slab at slab=%p, cache=%p, order=%zu", slab, cache, order);
    slab->num_objects = cache->num_objects;
    slab->free_count = slab->num_objects;
    size_t available_space = (SLAB_PAGE_SIZE * (1 << order)) - sizeof(slab_t);
    return available_space >= cache->object_size * slab->num_objects;
}

static void link_slab_to_cache(slab_t *slab, slab_cache_t *cache) {
    slab->next = cache->slab_list;
    cache->slab_list = slab;
}

static void initialize_slab_free_list(slab_t *slab, slab_cache_t *cache) {
    uint8_t *ptr = (uint8_t *)(slab + 1);
    for (size_t i = 0; i < slab->num_objects; i++) {
        *(uint8_t **)ptr = cache->free_list;
        cache->free_list = ptr;
        ptr += cache->object_size;
    }
    cache->free_count = slab->num_objects;
}

static bool initialize_slab_cache(slab_cache_t *cache, size_t size, size_t order) {
    cache->object_size = size;
    cache->num_objects = ((SLAB_PAGE_SIZE * (1 << order)) / size);
    if (cache->num_objects == 0) return false;

    cache->free_count = 0;
    cache->free_list = NULL;
    cache->slab_list = NULL;
    return true;
}

static slab_cache_t *create_slab_cache(size_t size) {
    if (size == 0) return NULL;
    
    size_t order = get_order(size);
    slab_cache_t *cache = (slab_cache_t *)pmm_alloc_page();
    if (!cache) return NULL;

    if (!initialize_slab_cache(cache, size, order)) {
        pmm_free_pages(cache, 0, 1);
        return NULL;
    }

    return cache;
}

static slab_t *create_slab(slab_cache_t *cache, size_t order) {
    if (!cache) return NULL;

    slab_t *slab = (slab_t *)pmm_alloc_page();
    if (!slab) return NULL;

    if (!initialize_slab(slab, cache, order)) {
        pmm_free_pages(slab, order , 1);
        PANIC_FAILED_TO_CREATE_SLAB(*((uint32_t *)slab));
        return NULL;
    }

    link_slab_to_cache(slab, cache);
    initialize_slab_free_list(slab, cache);

    return slab;
}

static int get_slab_size(slab_cache_t *cache) {
    return cache->object_size * cache->num_objects;
}

static int get_slab_order(slab_cache_t *cache) {
    return get_order(get_slab_size(cache));
}

static void log_slab_info(slab_cache_t *cache) {
    log("Slab cache info:\n", LIGHT_GRAY);
    log("Object size: ", LIGHT_GRAY);
    log_uint("", cache->object_size);
    log("\nNumber of objects: ", LIGHT_GRAY);
    log_uint("", cache->num_objects);
    log("\nFree count: ", LIGHT_GRAY);
    log_uint("", cache->free_count);
    log("\nSlab list: ", LIGHT_GRAY);
    log_uint("", (uintptr_t)cache->slab_list);
    log("\nFree list: ", LIGHT_GRAY);
    log_uint("", (uintptr_t)cache->free_list);
    log("\n", LIGHT_GRAY);
}

static void* allocate_from_free_list(slab_cache_t *cache) {
    if (cache->free_count > 0 && cache->free_list) {
        void *ptr = cache->free_list;
        cache->free_list = *(uint8_t **)ptr;
        cache->free_count--;
        return ptr;
    }
    return NULL;
}

static slab_cache_t* validate_allocation(size_t size, size_t *out_order) {
    if (size == 0 || size > SLAB_MAX_SIZE) return NULL;

    size_t order = get_order(size);
    if (order >= SLAB_ORDER_COUNT) return NULL;

    *out_order = order;
    return slab_caches[order];
}

static void* create_and_allocate(slab_cache_t *cache, size_t order) {
    if (!create_slab(cache, order)) return NULL;
    return allocate_from_free_list(cache);
}

static void remove_slab_from_cache(slab_cache_t *cache, slab_t *slab) {
    if (slab == cache->slab_list) {
        cache->slab_list = slab->next;
        return;
    }

    for (slab_t *prev = cache->slab_list; prev; prev = prev->next) {
        if (prev->next == slab) {
            prev->next = slab->next;
            return;
        }
    }
}

static bool find_slab_in_cache(slab_cache_t *cache, slab_t *target_slab) {
    for (slab_t *slab = cache->slab_list; slab; slab = slab->next) {
        if (slab == target_slab) return true;
    }
    return false;
}

static slab_cache_t* find_containing_cache(slab_t *containing_slab, size_t *out_order) {
    for (size_t order = 0; order < SLAB_ORDER_COUNT; order++) {
        slab_cache_t *cache = slab_caches[order];
        if (!cache) continue;

        if (find_slab_in_cache(cache, containing_slab)) {
            *out_order = order;
            return cache;
        }
    }
    return NULL; // slab not found
}
static void add_to_free_list(slab_cache_t *cache, void *ptr) {
    *(uint8_t **)ptr = cache->free_list;
    cache->free_list = ptr;
    cache->free_count++;
}

static void destroy_slab(slab_t *slab, size_t order) {
    pmm_free_pages(slab, order, 1);
}

static bool is_slab_empty(slab_t *slab) {
    return slab->free_count == slab->num_objects;
}

static bool is_ptr_valid(void *ptr, uintptr_t page_addr) {
    uintptr_t ptr_addr = (uintptr_t)ptr;
    return ptr && ptr_addr >= page_addr + sizeof(slab_t);
}

 slab_t* get_containing_slab(void *ptr) {
    uintptr_t ptr_addr = (uintptr_t)ptr;
    return (slab_t *)(ptr_addr & ~(SLAB_PAGE_SIZE - 1));
}

static void initialize_slab_cache_for_order(size_t order) {
    size_t size = SLAB_MIN_SIZE << order;
    if (size == 0) return;  // Prevent overflow

    slab_caches[order] = create_slab_cache(size);
    if (slab_caches[order]) {
        log("Initialized slab cache for size: ", GREEN);
        log_uint("", size);
    }
}

void slab_init(void) {
    log("Initializing slab allocator...\n", LIGHT_GRAY);

    for (size_t i = 0; i < SLAB_ORDER_COUNT; i++) {
        initialize_slab_cache_for_order(i);
    }
    log("Slab allocator initialized. \n", GREEN);
}


// not thread safe
// use through a wrapper or kmalloc()
void *slab_alloc(size_t size) {
    size_t order;
    slab_cache_t *cache = validate_allocation(size, &order);
    if (!cache) return NULL;

    void *ptr = allocate_from_free_list(cache);
    return ptr ? ptr : create_and_allocate(cache, order);
}

static void handle_slab_free(void *ptr, slab_t *containing_slab, slab_cache_t *cache, size_t order) {
    add_to_free_list(cache, ptr);
    containing_slab->free_count++;

    if (is_slab_empty(containing_slab)) {
        remove_slab_from_cache(cache, containing_slab);
        destroy_slab(containing_slab, order);
    }
}

// not thread safe
// use through a wrapper or kmalloc()
void slab_free(void *ptr) {
    if (!ptr) return;

    slab_t *containing_slab = get_containing_slab(ptr);
    if (!is_ptr_valid(ptr, (uintptr_t)containing_slab)) return;

    size_t order;
    slab_cache_t *cache = find_containing_cache(containing_slab, &order);
    if (!cache) return;

    handle_slab_free(ptr, containing_slab, cache, order);
}

static void* handle_reallocation(void *ptr, size_t new_size) {
    void *new_ptr = slab_alloc(new_size);
    if (!new_ptr) return NULL;

    slab_t *old_slab = get_containing_slab(ptr);
    size_t old_order;
    slab_cache_t *old_cache = find_containing_cache(old_slab, &old_order);
    
    if (old_cache) {
        flop_memcpy(new_ptr, ptr, MIN(new_size, old_cache->object_size));
    }
    
    slab_free(ptr);
    return new_ptr;
}

void* slab_realloc(void *ptr, size_t new_size) {
    if (!ptr) return slab_alloc(new_size);
    if (!new_size) {
        slab_free(ptr);
        return NULL;
    }

    return handle_reallocation(ptr, new_size);
}

void* slab_calloc(size_t num, size_t size) {
    size_t total = num * size;
    void *ptr = slab_alloc(total);
    if (ptr) flop_memset(ptr, 0, total);
    return ptr;
}

static void* align_pointer(void *ptr, size_t alignment) {
    uintptr_t addr = (uintptr_t)ptr;
    return (void*)((addr + alignment - 1) & ~(alignment - 1));
}

static bool is_valid_alignment(size_t alignment) {
    return alignment && !(alignment & (alignment - 1));
}

void* slab_aligned_alloc(size_t alignment, size_t size) {
    if (!is_valid_alignment(alignment)) {
        log("slab: invalid alignment\n", RED);
    }
    
    size_t padded_size = size + alignment - 1;
    void *ptr = slab_alloc(padded_size);
    if (!ptr) return NULL;
    
    return align_pointer(ptr, alignment);
}

size_t slab_get_allocated_size(void *ptr) {
    if (!ptr) return 0;
    
    slab_t *slab = get_containing_slab(ptr);
    size_t order;
    slab_cache_t *cache = find_containing_cache(slab, &order);
    
    return cache ? cache->object_size : 0;
}

static void* handle_resize(void *ptr, size_t new_size) {
    slab_t *slab = get_containing_slab(ptr);
    size_t order;
    slab_cache_t *cache = find_containing_cache(slab, &order);
    if (!cache) return NULL;

    void *new_ptr = slab_alloc(new_size);
    if (new_ptr) {
        flop_memcpy(new_ptr, ptr, cache->object_size);
        slab_free(ptr);
    }
    return new_ptr;
}

void *slab_resize(void *ptr, size_t new_size) {
    if (!ptr) return slab_alloc(new_size);
    if (!new_size) {
        slab_free(ptr);
        return NULL;
    }

    return handle_resize(ptr, new_size);
}

static void print_slab_cache_info(slab_cache_t *cache) {
    if (!cache) return;

    log_f("Slab cache size:", cache->object_size);
    log_f("  Free count:", cache->free_count);

    for (slab_t *slab = cache->slab_list; slab; slab = slab->next) {
        log_f("    Slab free count:", slab->free_count);
    }
}

void slab_debug(void) {
    for (size_t i = 0; i < SLAB_ORDER_COUNT; i++) {
        print_slab_cache_info(slab_caches[i]);
    }
}