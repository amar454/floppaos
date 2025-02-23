#ifndef KMALLOC_H
#define KMALLOC_H

#include <stddef.h>
#include <stdint.h>



// Function to allocate a block of memory
void* kmalloc(uint32_t size);

// Function to free a previously allocated block of memory
void kfree(void* ptr);

#endif // KMALLOC_H