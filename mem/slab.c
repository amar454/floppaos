#include "slab.h"
#include "pmm.h"
#include "../lib/logging.h"
#include "../drivers/vga/vgahandler.h"
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

void slab_free(void *ptr) {
    if (!ptr) return;

    // Find which slab order the object belongs to
    uintptr_t ptr_addr = (uintptr_t)ptr;
    uintptr_t page_addr = ptr_addr & ~(SLAB_PAGE_SIZE - 1);
    slab_t *containing_slab = (slab_t *)page_addr;

    // Validate pointer is within a valid slab range
    if (ptr_addr < page_addr + sizeof(slab_t)) return;

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
                // Found the correct cache
                *(uint8_t **)ptr = cache->free_list;
                cache->free_list = ptr;
                cache->free_count++;
                slab->free_count++;

                // If the slab becomes fully free, release back to buddy system
                if (slab->free_count == slab->num_objects) {
                    if (slab == cache->slab_list) {
                        cache->slab_list = slab->next;
                    } else {
                        // Find and remove slab from list
                        slab_t *prev = cache->slab_list;
                        while (prev && prev->next != slab) {
                            prev = prev->next;
                        }
                        if (prev) {
                            prev->next = slab->next;
                        }
                    }
                    pmm_free_pages(slab, order);
                }
                return;
            }
            slab = slab->next;
        }
        order++;
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
