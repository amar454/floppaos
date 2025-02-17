#ifndef KALLOC_H
#define KALLOC_H

#include <stddef.h>

void* kalloc(size_t size);
void kfree(void* ptr);
void* krealloc(void* ptr, uint32_t size);
void* kcalloc(uint32_t num, uint32_t size); 
#endif // KALLOC_H 