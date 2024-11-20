#ifndef MEMUTILS_H
#define MEMUTILS_H

#include <stdint.h>
#include <stddef.h>

// Function to allocate memory
void *flop_malloc(size_t size);
void *flop_memset(void *dest, int value, size_t size);
// Function to free allocated memory
void flop_free(void *ptr);
int flop_memcmp(const void *ptr1, const void *ptr2, size_t num) ;
// Function to copy memory
void *flop_memcpy(void *dest, const void *src, size_t n);

#endif // MEMUTILS_H
