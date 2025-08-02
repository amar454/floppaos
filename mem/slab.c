#include <stdint.h>
#include <stddef.h>
#include "paging.h"
#include "../lib/logging.h"
#include "../lib/assert.h"
#include "pmm.h"
#include "utils.h"

#define PAGE_SIZE 4096
#define SLAB_MIN_SHIFT 4
#define SLAB_MAX_SHIFT 12
#define SLAB_CLASS_COUNT (SLAB_MAX_SHIFT - SLAB_MIN_SHIFT + 1)
#define SLAB_METADATA_PER_PAGE (PAGE_SIZE / sizeof(slab_t))

typedef struct slab {
    struct slab* next;
    void* freelist;
    void* data;
    size_t free_count;
} slab_t;

typedef struct slab_cache {
    slab_t* slabs;
    size_t obj_size;
    size_t obj_count;
} slab_cache_t;

static slab_cache_t slab_caches[SLAB_CLASS_COUNT];




// memory pool for allocating slab structs
// this prevents slab allocations for slab structs because that's retarded
// the pool is made up of a linked list of physical pages
// these are divided by the size of the slab_t struct, 
// we will start off with one physical page
// we will do a check in _slab_pool_alloc to see if we need to allocate a new page
// if we do, alloc a new page and append it to the linked list
typedef struct slab_pool_page {
    struct slab_pool_page* next;
    slab_t slab_array[]; // array of slab_t structs, size is determined by SLAB_METADATA_PER_PAGE
} slab_pool_page_t;

static slab_pool_page_t* slab_pool_head = NULL; // head of linked list
static uint32_t slab_meta_used = 0;
static slab_t* current_slab_array = NULL;

static void _slab_pool_init() {
    slab_pool_head = (slab_pool_page_t*)pmm_alloc_page();
    if (!slab_pool_head) return;

    slab_pool_head->next = NULL;
    current_slab_array = slab_pool_head->slab_array;
    slab_meta_used = 0;
}

static inline  __attribute__((always_inline)) int _slab_pool_need_new_page() {
    return (!current_slab_array || slab_meta_used >= SLAB_METADATA_PER_PAGE);
}

static int _slab_pool_add_page() {
    slab_pool_page_t* new_page = (slab_pool_page_t*)pmm_alloc_page();
    if (!new_page) return 0;

    new_page->next = NULL;

    // Append to the linked list
    slab_pool_page_t* iter = slab_pool_head;
    while (iter->next) iter = iter->next;
    iter->next = new_page;

    current_slab_array = new_page->slab_array;
    slab_meta_used = 0;
    return 1;
}
static slab_t* _slab_pool_alloc() {
    if (_slab_pool_need_new_page()) 
    {
        if (!_slab_pool_add_page()) return NULL;
    }
    return &current_slab_array[slab_meta_used++];
}
static slab_t* _create_slab(slab_cache_t* cache) {
    void* mem = pmm_alloc_page(); // allocate the actual slab
    if (!mem) return NULL;

    slab_t* slab = _slab_pool_alloc(); // add slab struct to pool
    if (!slab) return NULL;

    slab->data = mem; 
    slab->freelist = NULL;
    slab->free_count = cache->obj_count;
    slab->next = NULL;

    uintptr_t ptr = (uintptr_t)mem;
    for (size_t i = 0; i < cache->obj_count; i++) { // add objs to free list
        void* obj = (void*)(ptr + i * cache->obj_size);
        *(void**)obj = slab->freelist;
        slab->freelist = obj;
    }
    return slab;
}

static void* _slab_cache_alloc(slab_cache_t* cache) {
    // find a slab in cache with free objects
    for (slab_t* slab = cache->slabs; slab; slab = slab->next) { 
        if (slab->freelist) { 
            // if we found a slab with free objects
            // remove the object from the freelist
            void* obj = slab->freelist;
            slab->freelist = *(void**)obj;
            slab->free_count--;
            return obj;
        }
    }
    // if no slabs with free list found, create a new slab
    slab_t* new_slab = _create_slab(cache);
    if (!new_slab) return NULL;

    new_slab->next = cache->slabs;
    cache->slabs = new_slab;

    void* obj = new_slab->freelist;
    new_slab->freelist = *(void**)obj;
    new_slab->free_count--;

    return obj;
}

static void _slab_cache_free(slab_cache_t* cache, void* ptr) {
    for (slab_t* slab = cache->slabs; slab; slab = slab->next) {
        uintptr_t start = (uintptr_t)slab->data;
        uintptr_t end = start + PAGE_SIZE;
        if ((uintptr_t)ptr >= start && (uintptr_t)ptr < end) {
            *(void**)ptr = slab->freelist;
            slab->freelist = ptr;
            slab->free_count++;
            return;
        }
    }
}
void* slab_alloc(size_t size) {
    for (int i = 0; i < SLAB_CLASS_COUNT; i++) {
        if (size <= slab_caches[i].obj_size) {
            return _slab_cache_alloc(&slab_caches[i]);
        }
    }
    return NULL;
}

void slab_free(void* ptr, size_t size) {
    for (int i = 0; i < SLAB_CLASS_COUNT; i++) {
        if (size <= slab_caches[i].obj_size) {
            _slab_cache_free(&slab_caches[i], ptr);
            return;
        }
    }
}
void slab_init() {
    for (int i = 0; i < SLAB_CLASS_COUNT; i++) {
        size_t obj_size = 1UL << (i + SLAB_MIN_SHIFT);
        size_t objs_per_page = PAGE_SIZE / obj_size;

        slab_caches[i].obj_size = obj_size;
        slab_caches[i].obj_count = objs_per_page;
        slab_caches[i].slabs = NULL;
    }

   _slab_pool_init();

}
