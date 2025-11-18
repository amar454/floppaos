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
#include "slab.h"
#include "pmm.h"
#include "../lib/logging.h"
#include "../kernel/kernel.h"
#include "../drivers/vga/vgahandler.h"
#include "utils.h"
#include <stdbool.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static slab_cache_t slab_cache_array[SLAB_ORDER_COUNT];
static slab_cache_t* slab_caches[SLAB_ORDER_COUNT] = {0};

static size_t slab_get_order(size_t size) {
    if (!size)
        return 0;
    size_t order = 0;
    size_t slab_bytes = SLAB_PAGE_SIZE;
    while (slab_bytes < size + sizeof(slab_t)) {
        slab_bytes <<= 1;
        order++;
        if (order >= SLAB_ORDER_COUNT - 1)
            break;
    }
    return order;
}

static slab_cache_t* slab_create_cache_for_order(size_t order) {
    size_t size = SLAB_MIN_SIZE << order;
    slab_cache_t* cache = &slab_cache_array[order];
    if (!cache)
        return NULL;
    cache->object_size = size;
    cache->num_objects = (SLAB_PAGE_SIZE * (1u << order) - sizeof(slab_t)) / size;
    if (!cache->num_objects)
        return NULL;
    cache->free_list = NULL;
    cache->slab_list = NULL;
    cache->free_count = 0;
    return cache;
}

static slab_t* slab_create(slab_cache_t* cache, size_t order) {
    if (!cache)
        return NULL;
    slab_t* slab = (slab_t*) pmm_alloc_pages(0, 1u << order);
    if (!slab)
        return NULL;
    size_t slab_bytes = SLAB_PAGE_SIZE * (1u << order);
    slab->num_objects = (slab_bytes - sizeof(slab_t)) / cache->object_size;
    if (!slab->num_objects) {
        pmm_free_pages(slab, order, 1);
        return NULL;
    }
    slab->free_count = slab->num_objects;
    slab->next = cache->slab_list;
    cache->slab_list = slab;
    uint8_t* ptr = (uint8_t*) (slab + 1);
    for (size_t i = 0; i < slab->num_objects; i++) {
        *(uint8_t**) ptr = cache->free_list;
        cache->free_list = ptr;
        ptr += cache->object_size;
    }
    cache->free_count += slab->num_objects;
    return slab;
}

static void* slab_free_list_alloc(slab_cache_t* cache) {
    if (!cache || !cache->free_list)
        return NULL;
    void* ptr = cache->free_list;
    cache->free_list = *(uint8_t**) ptr;
    cache->free_count--;
    return ptr;
}

static slab_cache_t* slab_find_cache_from_size(size_t size, size_t* out_order) {
    if (!size || size > SLAB_MAX_SIZE || !out_order)
        return NULL;
    for (size_t i = 0; i < SLAB_ORDER_COUNT; i++) {
        if ((SLAB_MIN_SIZE << i) >= size) {
            *out_order = i;
            return slab_caches[i];
        }
    }
    return NULL;
}

void slab_init(void) {
    for (size_t i = 0; i < SLAB_ORDER_COUNT; i++) {
        slab_caches[i] = slab_create_cache_for_order(i);
    }
}

void* slab_alloc(size_t size) {
    if (!size || size > SLAB_MAX_SIZE)
        return NULL;
    size_t order;
    slab_cache_t* cache = slab_find_cache_from_size(size, &order);
    if (!cache)
        return NULL;
    void* ptr = slab_free_list_alloc(cache);
    if (ptr)
        return ptr;
    return slab_create(cache, order) ? slab_free_list_alloc(cache) : NULL;
}

static slab_t* get_containing_slab(void* ptr) {
    return (slab_t*) ((uintptr_t) ptr & ~(SLAB_PAGE_SIZE - 1));
}

static slab_cache_t* slab_find_containing_cache(slab_t* slab, size_t* out_order) {
    if (!slab || !out_order)
        return NULL;
    for (size_t i = 0; i < SLAB_ORDER_COUNT; i++) {
        slab_cache_t* cache = slab_caches[i];
        for (slab_t* s = cache->slab_list; s; s = s->next) {
            if (s == slab) {
                *out_order = i;
                return cache;
            }
        }
    }
    return NULL;
}

static void slab_add_to_free_list(slab_cache_t* cache, void* ptr) {
    *(uint8_t**) ptr = cache->free_list;
    cache->free_list = ptr;
    cache->free_count++;
}

static void slab_remove_slab(slab_cache_t* cache, slab_t* slab) {
    if (cache->slab_list == slab) {
        cache->slab_list = slab->next;
        return;
    }
    for (slab_t* s = cache->slab_list; s; s = s->next) {
        if (s->next == slab) {
            s->next = slab->next;
            return;
        }
    }
}

void slab_free(void* ptr) {
    if (!ptr)
        return;
    slab_t* slab = get_containing_slab(ptr);
    size_t order;
    slab_cache_t* cache = slab_find_containing_cache(slab, &order);
    if (!cache)
        return;
    slab_add_to_free_list(cache, ptr);
    slab->free_count--;
    if (slab->free_count == slab->num_objects) {
        slab_remove_slab(cache, slab);
        pmm_free_pages(slab, order, 1);
    }
}

void* slab_calloc(size_t num, size_t size) {
    size_t total = num * size;
    void* ptr = slab_alloc(total);
    if (ptr)
        flop_memset(ptr, 0, total);
    return ptr;
}

void* slab_realloc(void* ptr, size_t new_size) {
    if (!ptr)
        return slab_alloc(new_size);
    if (!new_size) {
        slab_free(ptr);
        return NULL;
    }
    void* new_ptr = slab_alloc(new_size);
    if (!new_ptr)
        return NULL;
    slab_t* old_slab = get_containing_slab(ptr);
    size_t order;
    slab_cache_t* old_cache = slab_find_containing_cache(old_slab, &order);
    if (old_cache)
        flop_memcpy(new_ptr, ptr, MIN(new_size, old_cache->object_size));
    slab_free(ptr);
    return new_ptr;
}

void* slab_aligned_alloc(size_t alignment, size_t size) {
    if (!alignment || (alignment & (alignment - 1)))
        return NULL;
    void* ptr = slab_alloc(size + alignment - 1);
    if (!ptr)
        return NULL;
    uintptr_t addr = ((uintptr_t) ptr + alignment - 1) & ~(alignment - 1);
    return (void*) addr;
}

size_t slab_get_allocated_size(void* ptr) {
    if (!ptr)
        return 0;
    slab_t* slab = get_containing_slab(ptr);
    size_t order;
    slab_cache_t* cache = slab_find_containing_cache(slab, &order);
    return cache ? cache->object_size : 0;
}

void* slab_resize(void* ptr, size_t new_size) {
    return slab_realloc(ptr, new_size);
}
