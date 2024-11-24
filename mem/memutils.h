#ifndef MEMUTILS_H
#define MEMUTILS_H

#include <stddef.h>

// Memory initialization
void init_memory();

// Custom memory management functions
void *flop_malloc(size_t size);
void flop_free(void *ptr);

// Memory manipulation functions
void *flop_memset(void *dest, int value, size_t size);
int flop_memcmp(const void *ptr1, const void *ptr2, size_t num);
void *flop_memcpy(void *dest, const void *src, size_t n);
void *flop_memmove(void *dest, const void *src, size_t n);

#endif // MEMUTILS_H
