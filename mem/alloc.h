#ifndef ALLOC_H
#define ALLOC_H
#include "pmm.h"
#include <complex.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#define HEAP_PERCENTAGE 25  // Use 25% of total memory for heap
#define MIN_HEAP_SIZE (4UL * 1024 * 1024)  // 4MB minimum
#define MAX_HEAP_SIZE (512UL * 1024 * 1024)  // 512MB maximum
#define KERNEL_HEAP_START 0xC0400000  // Starting virtual address for kernel heap
#define PAGE_SIZE 4096
void* kmalloc(size_t size);
void kfree(void* ptr, size_t size);
void* kcalloc(uint32_t num, size_t size);

void *krealloc(void *ptr, size_t old_size, size_t new_size) ;
void init_kernel_heap(void);
void test_alloc();
#endif // KMALLOC_H