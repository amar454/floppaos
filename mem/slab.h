#ifndef SLAB_H
#define SLAB_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "paging.h"
#include "../lib/logging.h"
#include "pmm.h"
#include "vmm.h"
#include "../drivers/vga/vgahandler.h"
#include "../lib/str.h"

#include "utils.h"

// Constants
#define SLAB_PAGE_SIZE 4096  // 4 KB page size
#define SLAB_MIN_SIZE 8      // Minimum object size
#define SLAB_MAX_SIZE (SLAB_PAGE_SIZE / 2)  // Maximum object size
#define SLAB_ORDER_COUNT 10  // Maximum number of slab orders

// Macros
#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// Slab structure
typedef struct slab {
    size_t num_objects;       // Number of objects in the slab
    size_t free_count;        // Number of free objects in the slab
    struct slab *next;        // Pointer to the next slab in the cache
    size_t order;             // Order of the slab (size in powers of 2)
} slab_t;

// Slab cache structure
typedef struct slab_cache {
    size_t object_size;       // Size of each object in the cache
    size_t num_objects;       // Number of objects per slab
    size_t free_count;        // Number of free objects in the cache
    void *free_list;          // Pointer to the free list
    slab_t *slab_list;        // Pointer to the list of slabs
} slab_cache_t;

// Function declarations

// Slab allocator initialization
void slab_init(void);

// Slab allocation and deallocation
void *slab_alloc(size_t size);
void slab_free(void *ptr);

// Slab reallocation
void *slab_realloc(void *ptr, size_t new_size);
void *slab_calloc(size_t num, size_t size);
void *slab_aligned_alloc(size_t alignment, size_t size);


// Slab mapping functions
void map_slab_to_page_directory(PDE *page_directory, slab_t *slab);
void unmap_slab_from_page_directory(PDE *page_directory, slab_t *slab);

// Slab debugging
void slab_debug(void);

// Utility functions
slab_t* get_containing_slab(void *ptr);
size_t slab_get_allocated_size(void *ptr);
void *slab_resize(void *ptr, size_t new_size);

#endif // SLAB_H