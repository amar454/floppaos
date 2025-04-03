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
// Function prototypes

// Initialization functions
void init_kernel_heap(void);

// Allocation and deallocation
void *kmalloc(size_t size);
void kfree(void *ptr, size_t size);
void *kcalloc(uint32_t num, size_t size);
void *krealloc(void *ptr, size_t old_size, size_t new_size);
void *kaligned_alloc(size_t size, size_t alignment);

// Memory management
void add_memory_block(uintptr_t start, size_t size, int is_free);
void free_memory_block(void *ptr, size_t size);

// Slab management
void *map_slab_to_heap(void *virt_addr, void *slab_ptr);
void unmap_slab_from_heap(void *heap_addr, size_t size);
void *slab_alloc(size_t size);
void slab_free(void *ptr);

// Debugging functions
void dump_heap(void);
void check_heap_integrity(void);

// Heap expansion and shrinking
void expand_kernel_heap(size_t additional_size);
void shrink_kernel_heap(size_t reduce_size);

// Memory checking
void mem_check_task(void);

#endif // ALLOC_H