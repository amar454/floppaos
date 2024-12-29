#ifndef MEMUTILS_H
#define MEMUTILS_H

#include <stddef.h>
#include <stdint.h>
#define MEMORY_SIZE 1024 * 1024 * 16 // Size of the simulated memory pool
#define ALIGNMENT 4 // 4-byte alignment for memory blocks
#define HEADER_SIZE sizeof(FlopMemBlockHeader)
#define PAGE_SIZE 4096 // Size of a virtual memory page

// Memory initialization
void init_memory();
// Free list structure
typedef struct FlopMemBlockHeader {
    size_t size;                        // Size of the block (excluding header)
    struct FlopMemBlockHeader *next;    // Pointer to the next free block
} FlopMemBlockHeader;

// Virtual memory page structure
typedef struct VirtualPage {
    uint8_t *physical_address;          // Physical address the virtual page maps to
    int is_allocated;                   // Allocation status
} VirtualPage;
// Custom memory management functions
void *flop_malloc(size_t size);
void flop_free(void *ptr);

// Memory manipulation functions
void *flop_memset(void *dest, int value, size_t size);
int flop_memcmp(const void *ptr1, const void *ptr2, size_t num);
void *flop_memcpy(void *dest, const void *src, size_t n);
void *flop_memmove(void *dest, const void *src, size_t n);
void *allocate_page();
void free_page(void *virtual_address);
#endif // MEMUTILS_H
