#ifndef MEMUTILS_H
#define MEMUTILS_H

#include <stdint.h>
#include <stddef.h>

// Function to allocate memory
void *flop_malloc(size_t size);

// Function to free allocated memory
void flop_free(void *ptr);

// Function to copy memory
void *flop_memcpy(void *dest, const void *src, size_t n);

#endif // MEMUTILS_H
