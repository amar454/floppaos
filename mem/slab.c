
#include "slab.h"
#include "pmm.h"
#include "../lib/logging.h"
#include "../kernel.h"
#include "../drivers/vga/vgahandler.h"
#include "utils.h"
#include <stdbool.h>

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

// 
static bool initialize_slab(slab_t *slab, slab_cache_t *cache, size_t order) {
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
    slab_cache_t *cache = (slab_cache_t *)pmm_alloc_pages(0);
    if (!cache) return NULL;

    if (!initialize_slab_cache(cache, size, order)) {
        pmm_free_pages(cache, 0);
        return NULL;
    }

    return cache;
}



// Slab management functions
static slab_t *create_slab(slab_cache_t *cache, size_t order) {
    if (!cache) return NULL;

    slab_t *slab = (slab_t *)pmm_alloc_pages(order);
    if (!slab) return NULL;

    if (!initialize_slab(slab, cache, order)) {
        pmm_free_pages(slab, order);
        PANIC_FAILED_TO_CREATE_SLAB(*((uint32_t *)slab));
        return NULL;
    }

    link_slab_to_cache(slab, cache);
    initialize_slab_free_list(slab, cache);

    return slab;
}


// Memory allocation functions
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

// Memory deallocation functions
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

// Memory management helper functions
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
static void initialize_slab_cache_for_order(size_t order) {
    size_t size = SLAB_MIN_SIZE << order;
    if (size == 0) return;  // Prevent overflow

    slab_caches[order] = create_slab_cache(size);
    if (slab_caches[order]) {
        log_step("Initialized slab cache for size: ", GREEN);
        log_uint("", size);
    }
}

void slab_init(void) {
    log_step("Initializing slab allocator", LIGHT_GRAY);

    for (size_t i = 0; i < SLAB_ORDER_COUNT; i++) {
        initialize_slab_cache_for_order(i);
    }
}



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
    if (!is_valid_alignment(alignment)) return NULL;
    
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

