#ifndef MEMUTILS_H
#define MEMUTILS_H

#include <stddef.h>

// Memory initialization function
void init_memory();

// Memory allocation functions
void *flop_malloc(size_t size);
void flop_free(void *ptr);
void *flop_realloc(void *ptr, size_t size);
void *flop_malloc_best_fit(size_t size);
void *flop_malloc_buddy(size_t size);

// Custom memory functions
void *flop_memset(void *dest, int value, size_t size);
int flop_memcmp(const void *ptr1, const void *ptr2, size_t num);
void *flop_memcpy(void *dest, const void *src, size_t n);
void *flop_memmove(void *dest, const void *src, size_t n);
void *flop_memchr(const void *ptr, int value, size_t num);

// Slab allocator functions
void *slab_malloc(size_t size);
void slab_free(void *ptr);

// Garbage Collection-based functions
void *gc_malloc(size_t size);
void gc_free(void *ptr);
void gc_collect();

// GC Helper functions
void gc_mark(void *ptr);
void gc_sweep();

// Root management for GC
void gc_add_root(void *ptr);

#endif // MEMUTILS_H
