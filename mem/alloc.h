#ifndef ALLOC_H
#define ALLOC_H
#include "pmm.h"
#include "paging.h"
#include <complex.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#define PAGE_SIZE           4096
#define KERNEL_VADDR_BASE   0xC0000000
#define USER_VADDR_BASE     0x00000000
#define KERNEL_HEAP_START   0xC8000000  
#define MAX_HEAP_SIZE       (128 * 1024 * 1024) 
#define MIN_HEAP_SIZE       (4 * 1024 * 1024)   
#define HEAP_PERCENTAGE     10                 

void* kmalloc(size_t size);
void kfree(void* ptr, size_t size);
void* kcalloc(uint32_t num, size_t size);

void *krealloc(void *ptr, size_t old_size, size_t new_size) ;
void init_kernel_heap(void);
void test_alloc();
#endif // KMALLOC_H