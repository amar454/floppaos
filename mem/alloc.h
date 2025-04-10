#ifndef ALLOC_H
#define ALLOC_H
#include "pmm.h"
#include "paging.h"
#include <complex.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#define HEAP_PERCENTAGE 25  // Use 25% of total memory for heap
#define MIN_HEAP_SIZE (4UL * 1024 * 1024)  // 4MB minimum
#define MAX_HEAP_SIZE (512UL * 1024 * 1024)  // 512MB maximum
#define KERNEL_HEAP_START 0xC0400000  // Starting virtual address for kernel heap
#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))
#define PMM_RETURN_THRESHOLD (8 * PAGE_SIZE) // Return to PMM if free block is this big

void* kmalloc(size_t size);
void kfree(void* ptr, size_t size);
void* kcalloc(uint32_t num, size_t size);

void *krealloc(void *ptr, size_t old_size, size_t new_size) ;
void init_kernel_heap(void);
void destroy_kernel_heap(PDE *kernel_page_directory);
void test_alloc();
#endif // KMALLOC_H