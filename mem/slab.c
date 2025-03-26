#include "slab.h"
#include "paging.h"
#include "pmm.h"
#include "../lib/logging.h"
#include "../drivers/vga/vgahandler.h"
#include "utils.h"
#include "vmm.h"
#include <stdbool.h>
#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))

static slab_cache_t *slab_caches[SLAB_ORDER_COUNT];

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

static slab_t *create_slab(slab_cache_t *cache, size_t order) {
    if (!cache) return NULL;

    slab_t *slab = (slab_t *)pmm_alloc_pages(order);
    if (!slab) return NULL;

    slab->num_objects = cache->num_objects;
    slab->free_count = slab->num_objects;
    slab->next = cache->slab_list;
    cache->slab_list = slab;

    // Calculate available space after slab header
    size_t available_space = (SLAB_PAGE_SIZE * (1 << order)) - sizeof(slab_t);
    if (available_space < cache->object_size * slab->num_objects) {
        pmm_free_pages(slab, order);
        return NULL;
    }

    // Initialize the free list within the slab
    uint8_t *ptr = (uint8_t *)(slab + 1);
    for (size_t i = 0; i < slab->num_objects; i++) {
        *(uint8_t **)ptr = cache->free_list;
        cache->free_list = ptr;
        ptr += cache->object_size;
    }
    cache->free_count = slab->num_objects;

    return slab;
}


void slab_init(void) {
    log_step("Initializing slab allocator", LIGHT_GRAY);

    for (size_t i = 0; i < SLAB_ORDER_COUNT; i++) {
        size_t size = SLAB_MIN_SIZE << i;
        if (size == 0) continue;  // Prevent overflow
        slab_caches[i] = create_slab_cache(size);
        if (slab_caches[i]) {
            log_step("Initialized slab cache for size: ", GREEN);
            log_uint("", size);
        }
    }
}

void *slab_alloc(size_t size) {
    if (size == 0 || size > SLAB_MAX_SIZE) return NULL;

    size_t order = get_order(size);
    if (order >= SLAB_ORDER_COUNT) return NULL;

    slab_cache_t *cache = slab_caches[order];
    if (!cache) return NULL;

    // Try to allocate from existing free list
    if (cache->free_count > 0 && cache->free_list) {
        void *ptr = cache->free_list;
        cache->free_list = *(uint8_t **)ptr;
        cache->free_count--;
        return ptr;
    }

    // If empty, create a new slab
    if (!create_slab(cache, order)) return NULL;

    // Try again after creating slab
    if (cache->free_count == 0 || !cache->free_list) return NULL;
    void *ptr = cache->free_list;
    cache->free_list = *(uint8_t **)ptr;
    cache->free_count--;
    return ptr;
}


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
    return NULL;
}

static void add_to_free_list(slab_cache_t *cache, void *ptr) {
    *(uint8_t **)ptr = cache->free_list;
    cache->free_list = ptr;
    cache->free_count++;
}

void slab_free(void *ptr) {
    if (!ptr) return;

    uintptr_t ptr_addr = (uintptr_t)ptr;
    uintptr_t page_addr = ptr_addr & ~(SLAB_PAGE_SIZE - 1);
    slab_t *containing_slab = (slab_t *)page_addr;

    if (ptr_addr < page_addr + sizeof(slab_t)) return;

    size_t order;
    slab_cache_t *cache = find_containing_cache(containing_slab, &order);
    if (!cache) return;

    add_to_free_list(cache, ptr);
    containing_slab->free_count++;

    if (containing_slab->free_count == containing_slab->num_objects) {
        remove_slab_from_cache(cache, containing_slab);
        pmm_free_pages(containing_slab, order);
    }
}


void slab_debug(void) {
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

static void destroy_slab(slab_t *slab, size_t order) {
    pmm_free_pages(slab, order);
}

static bool is_slab_empty(slab_t *slab) {
    return slab->free_count == slab->num_objects;
}

static bool is_ptr_valid(void *ptr, uintptr_t page_addr) {
    uintptr_t ptr_addr = (uintptr_t)ptr;
    return ptr && ptr_addr >= page_addr + sizeof(slab_t);
}

static slab_t* get_containing_slab(void *ptr) {
    uintptr_t ptr_addr = (uintptr_t)ptr;
    return (slab_t *)(ptr_addr & ~(SLAB_PAGE_SIZE - 1));
}
#define MIN(a, b) ((a) < (b) ? (a) : (b))
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

    flop_memcpy(new_ptr, ptr, MIN(new_size, old_cache->object_size));
    slab_free(ptr);
    return new_ptr;
}

void* slab_calloc(size_t num, size_t size) {
    size_t total = num * size;
    void *ptr = slab_alloc(total);
    if (ptr) flop_memset(ptr, 0, total);
    return ptr;
}

void* slab_aligned_alloc(size_t alignment, size_t size) {
    if (!alignment || (alignment & (alignment - 1))) return NULL;
    size_t padded_size = size + alignment - 1;
    void *ptr = slab_alloc(padded_size);
    if (!ptr) return NULL;
    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t aligned = (addr + alignment - 1) & ~(alignment - 1);
    return (void*)aligned;
}

size_t slab_get_allocated_size(void *ptr) {
    if (!ptr) return 0;
    slab_t *slab = get_containing_slab(ptr);
    size_t order;
    slab_cache_t *cache = find_containing_cache(slab, &order);
    return cache ? cache->object_size : 0;
}

void *slab_resize(void *ptr, size_t new_size) {
    if (!ptr) return slab_alloc(new_size);
    if (!new_size) {
        slab_free(ptr);
        return NULL;
    }
    slab_t *slab = get_containing_slab(ptr);
    size_t order;
    slab_cache_t *cache = find_containing_cache(slab, &order);
    if (!cache) return NULL;
    void *new_ptr = slab_alloc(new_size);
    flop_memcpy(new_ptr, ptr, cache->object_size);
    slab_free(ptr);
    return new_ptr;     
}
