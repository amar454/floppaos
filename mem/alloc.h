#ifndef ALLOC_H
#define ALLOC_H
#include "pmm.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

void* kmalloc(uint32_t size);
void kfree(void* ptr, size_t size);
void* kcalloc(uint32_t num, uint32_t size);

void* krealloc(void* ptr, uint32_t current_size, uint32_t size);

void test_alloc();
#endif // KMALLOC_H