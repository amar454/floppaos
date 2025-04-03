#ifndef SLAB_H
#define SLAB_H

#include <stddef.h>
#include <stdint.h>

#define SLAB_PAGE_SIZE     4096
#define SLAB_MIN_SIZE      16
#define SLAB_MAX_SIZE      4096
#define SLAB_ORDER_COUNT   8

typedef struct slab {
    size_t num_objects;
    size_t free_count;
    struct slab *next;
} slab_t;

typedef struct slab_cache {
    size_t object_size;
    size_t num_objects;
    size_t free_count;
    void *free_list;
    slab_t *slab_list;
} slab_cache_t;

void slab_init(void);
void *slab_alloc(size_t size);
void slab_free(void *ptr);
void slab_debug(void);

#endif // SLAB_H

