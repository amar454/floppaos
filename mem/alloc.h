#ifndef ALLOC_H
#define ALLOC_H

#include <stddef.h>
#include <stdint.h>

void* malloc(uint32_t size);
void free(void* ptr);
void* calloc(uint32_t num, uint32_t size);
void* realloc(void* ptr, uint32_t size);
#endif // KMALLOC_H