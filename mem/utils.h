#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stddef.h>

// Function to set a block of memory to a specific value
void *flop_memset(void *dest, int value, size_t size);

// Function to compare two memory blocks
int flop_memcmp(const void *ptr1, const void *ptr2, size_t num);

// Function to copy a block of memory from source to destination
void *flop_memcpy(void *dest, const void *src, size_t n);

// Function to move a block of memory from source to destination
void *flop_memmove(void *dest, const void *src, size_t n);

#endif // UTILS_H
