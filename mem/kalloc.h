#ifndef KALLOC_H
#define KALLOC_H

#include <stddef.h>

<<<<<<< HEAD
// Function to initialize the memory allocator
void kalloc_init(void);

// Function to allocate a block of memory
void* kalloc(size_t size);

// Function to free a previously allocated block of memory
=======
void* kalloc(size_t size);
>>>>>>> parent of 02d85db (bs)
void kfree(void* ptr);

#endif // KALLOC_H