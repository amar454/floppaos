#include "memutils.h"
#include <stdint.h>
#include <stddef.h>
#include "../apps/echo.h"
#include "../drivers/vga/vgahandler.h"

#define SIMULATED_DISK_SIZE 1024 * 1024 // Size of the simulated memory pool
#define ALIGNMENT 4 // 4-byte alignment for memory blocks
#define HEADER_SIZE sizeof(FlopMemBlockHeader)
#define PAGE_SIZE 4096 // Size of a virtual memory page

// Simulated memory pool
static uint8_t simulated_memory[SIMULATED_DISK_SIZE];

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

static VirtualPage page_table[SIMULATED_DISK_SIZE / PAGE_SIZE];

// Head of the free list
static FlopMemBlockHeader *free_list = NULL;

// Align size to the nearest multiple of ALIGNMENT
static inline size_t align_size(size_t size) {
    return (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
}

// Initialize the memory allocator
void init_memory() {
    free_list = (FlopMemBlockHeader *)simulated_memory;
    free_list->size = SIMULATED_DISK_SIZE - HEADER_SIZE;
    free_list->next = NULL;

    // Initialize the page table
    for (size_t i = 0; i < SIMULATED_DISK_SIZE / PAGE_SIZE; i++) {
        page_table[i].physical_address = NULL;
        page_table[i].is_allocated = 0;
    }
}

// Translate virtual address to physical address
void *virtual_to_physical(void *virtual_address) {
    uintptr_t addr = (uintptr_t)virtual_address;
    size_t page_index = addr / PAGE_SIZE;
    if (page_table[page_index].is_allocated) {
        return page_table[page_index].physical_address + (addr % PAGE_SIZE);
    }
    return NULL;
}

// Allocate a virtual memory page
void *allocate_page() {
    for (size_t i = 0; i < SIMULATED_DISK_SIZE / PAGE_SIZE; i++) {
        if (!page_table[i].is_allocated) {
            page_table[i].is_allocated = 1;
            page_table[i].physical_address = &simulated_memory[i * PAGE_SIZE];
            return (void *)(i * PAGE_SIZE);
        }
    }
    echo("allocate_page: Out of pages!\n", RED);
    return NULL;
}

// Free a virtual memory page
void free_page(void *virtual_address) {
    uintptr_t addr = (uintptr_t)virtual_address;
    size_t page_index = addr / PAGE_SIZE;
    if (page_table[page_index].is_allocated) {
        page_table[page_index].is_allocated = 0;
        page_table[page_index].physical_address = NULL;
    } else {
        echo("free_page: Page not allocated!\n", RED);
    }
}

// Custom malloc implementation
void *flop_malloc(size_t size) {
    size = align_size(size); // Align size
    FlopMemBlockHeader *prev = NULL;
    FlopMemBlockHeader *current = free_list;

    while (current) {
        if (current->size >= size) {
            if (current->size >= size + HEADER_SIZE + ALIGNMENT) {
                // Split the block
                FlopMemBlockHeader *new_block = (FlopMemBlockHeader *)((uint8_t *)current + HEADER_SIZE + size);
                new_block->size = current->size - size - HEADER_SIZE;
                new_block->next = current->next;

                current->size = size;
                current->next = NULL;

                if (prev) {
                    prev->next = new_block;
                } else {
                    free_list = new_block;
                }
            } else {
                // Use the entire block
                if (prev) {
                    prev->next = current->next;
                } else {
                    free_list = current->next;
                }
            }
            return (void *)((uint8_t *)current + HEADER_SIZE);
        }
        prev = current;
        current = current->next;
    }

    echo("flop_malloc: Out of memory!\n", RED);
    return NULL;
}

// Custom free implementation
void flop_free(void *ptr) {
    if (!ptr) {
        echo("flop_free: NULL pointer!\n", RED);
        return;
    }

    FlopMemBlockHeader *block = (FlopMemBlockHeader *)((uint8_t *)ptr - HEADER_SIZE);
    FlopMemBlockHeader *current = free_list;

    // Insert the block into the free list
    if (!free_list || block < free_list) {
        block->next = free_list;
        free_list = block;
    } else {
        while (current->next && current->next < block) {
            current = current->next;
        }
        block->next = current->next;
        current->next = block;
    }

    // Merge with next block if adjacent
    if (block->next && (uint8_t *)block + HEADER_SIZE + block->size == (uint8_t *)block->next) {
        block->size += HEADER_SIZE + block->next->size;
        block->next = block->next->next;
    }

    // Merge with previous block if adjacent
    if (current && (uint8_t *)current + HEADER_SIZE + current->size == (uint8_t *)block) {
        current->size += HEADER_SIZE + block->size;
        current->next = block->next;
    }
}

// Custom memset implementation
void *flop_memset(void *dest, int value, size_t size) {
    uint8_t val = (uint8_t)value;
    asm volatile (
        "cld\n\t"
        "rep stosb"
        : "+D" (dest), "+c" (size)
        : "a" (val)
        : "memory"
    );
    return dest;
}

// Custom memcmp implementation
int flop_memcmp(const void *ptr1, const void *ptr2, size_t num) {
    if (!ptr1 || !ptr2) {
        echo("flop_memcmp: NULL pointer detected!\n", RED);
        return -1;
    }

    const uint8_t *p1 = (const uint8_t *)ptr1;
    const uint8_t *p2 = (const uint8_t *)ptr2;

    for (size_t i = 0; i < num; i++) {
        if (p1[i] != p2[i]) {
            return (p1[i] - p2[i]);
        }
    }
    return 0;
}

// Custom memcpy implementation
void *flop_memcpy(void *dest, const void *src, size_t n) {
    if (!dest || !src) {
        echo("flop_memcpy: NULL pointer detected!\n", RED);
        return NULL;
    }

    asm volatile (
        "cld\n\t"
        "rep movsb"
        : "+D" (dest), "+S" (src), "+c" (n)
        :
        : "memory"
    );
    return dest;
}

// Custom memmove implementation
void *flop_memmove(void *dest, const void *src, size_t n) {
    if (!dest || !src) {
        echo("flop_memmove: NULL pointer detected!\n", RED);
        return NULL;
    }

    if (dest < src) {
        return flop_memcpy(dest, src, n);
    } else {
        uint8_t *d = (uint8_t *)dest + n - 1;
        const uint8_t *s = (const uint8_t *)src + n - 1;

        asm volatile (
            "std\n\t"
            "rep movsb"
            : "+D" (d), "+S" (s), "+c" (n)
            :
            : "memory"
        );
        asm volatile ("cld" ::: "cc");
    }

    return dest;
}
