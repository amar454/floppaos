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

#define SLAB_PAGE_SIZE 4096  
#define SLAB_MIN_SIZE 8      
#define SLAB_MAX_SIZE (SLAB_PAGE_SIZE / 2)
#define SLAB_ORDER_COUNT 10  

// Macros
#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
  typedef struct slab {
      size_t num_objects;
      size_t free_count;
      struct slab *next;
      size_t order;
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


void *slab_realloc(void *ptr, size_t new_size);
void *slab_calloc(size_t num, size_t size);
void *slab_aligned_alloc(size_t alignment, size_t size);

void slab_debug(void);


slab_t* get_containing_slab(void *ptr);
size_t slab_get_allocated_size(void *ptr);
void *slab_resize(void *ptr, size_t new_size);
#endif // SLAB_H