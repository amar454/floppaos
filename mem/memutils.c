#include "memutils.h"
#include <stdint.h>
#include <stddef.h>
#include "../apps/echo.h"
#include "../drivers/vga/vgahandler.h"

// Memory Block Header Definition
#define HEAP_SIZE 1024 * 1024  // Size of the heap (1MB)
#define ALIGNMENT 8            // Align memory blocks to 8-byte boundary
#define HEADER_SIZE sizeof(FlopMemBlockHeader)

static uint8_t heap[HEAP_SIZE];  // Simulated heap area
static FlopMemBlockHeader *free_list = NULL;  // Head of the free list

// GC-specific data structures
static void **gc_roots = NULL;  // List of root references
static size_t gc_root_count = 0;

typedef struct FlopMemBlockHeader {
    size_t size;                   // Size of the block (excluding header)
    struct FlopMemBlockHeader *next;  // Pointer to the next free block
    int is_free;                    // Block state: 0 - used, 1 - free
    int marked;                     // Marked as alive for GC
} FlopMemBlockHeader;

// Align size to the nearest multiple of ALIGNMENT
static inline size_t align_size(size_t size) {
    return (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
}

// Initialize the memory allocator
void init_memory() {
    free_list = (FlopMemBlockHeader *)heap;
    free_list->size = HEAP_SIZE - HEADER_SIZE;
    free_list->next = NULL;
    free_list->is_free = 1;  // Mark the entire heap as free initially
    free_list->marked = 0;

    gc_root_count = 0;  // No roots initially
}

// Malloc Implementation (First Fit)
void *flop_malloc(size_t size) {
    size = align_size(size);
    FlopMemBlockHeader *prev = NULL;
    FlopMemBlockHeader *current = free_list;

    while (current) {
        if (current->is_free && current->size >= size) {
            if (current->size >= size + HEADER_SIZE + ALIGNMENT) {
                FlopMemBlockHeader *new_block = (FlopMemBlockHeader *)((uint8_t *)current + HEADER_SIZE + size);
                new_block->size = current->size - size - HEADER_SIZE;
                new_block->next = current->next;
                new_block->is_free = 1;
                new_block->marked = 0;

                current->size = size;
                current->next = new_block;
                current->is_free = 0;

                if (prev) {
                    prev->next = new_block;
                } else {
                    free_list = new_block;
                }
            } else {
                current->is_free = 0;
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

// Free Implementation (with merging adjacent blocks)
void flop_free(void *ptr) {
    if (!ptr) {
        echo("flop_free: NULL pointer!\n", RED);
        return;
    }

    FlopMemBlockHeader *block = (FlopMemBlockHeader *)((uint8_t *)ptr - HEADER_SIZE);
    block->is_free = 1;
    block->marked = 0;

    if (block->next && block->next->is_free) {
        block->size += HEADER_SIZE + block->next->size;
        block->next = block->next->next;
    }

    FlopMemBlockHeader *current = free_list;
    while (current && current->next != block) {
        current = current->next;
    }

    if (current && (uint8_t *)current + HEADER_SIZE + current->size == (uint8_t *)block) {
        current->size += HEADER_SIZE + block->size;
        current->next = block->next;
    } else {
        block->next = free_list;
        free_list = block;
    }
}

// Realloc Implementation
void *flop_realloc(void *ptr, size_t size) {
    if (!ptr) {
        return flop_malloc(size);
    }

    if (size == 0) {
        flop_free(ptr);
        return NULL;
    }

    void *new_ptr = flop_malloc(size);
    if (!new_ptr) {
        return NULL;
    }

    size_t old_size = ((FlopMemBlockHeader *)((uint8_t *)ptr - HEADER_SIZE))->size;
    size_t copy_size = (old_size < size) ? old_size : size;
    flop_memcpy(new_ptr, ptr, copy_size);
    flop_free(ptr);
    return new_ptr;
}

// Custom memory functions for kernel environment
void *flop_memset(void *dest, int value, size_t size) {
    uint8_t *ptr = dest;
    while (size--) {
        *ptr++ = (uint8_t)value;
    }
    return dest;
}

int flop_memcmp(const void *ptr1, const void *ptr2, size_t num) {
    const uint8_t *p1 = ptr1;
    const uint8_t *p2 = ptr2;
    while (num--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

void *flop_memcpy(void *dest, const void *src, size_t n) {
    uint8_t *d = dest;
    const uint8_t *s = src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

void *flop_memmove(void *dest, const void *src, size_t n) {
    uint8_t *d = dest;
    const uint8_t *s = src;
    if (d < s) {
        while (n--) {
            *d++ = *s++;
        }
    } else {
        d += n;
        s += n;
        while (n--) {
            *(--d) = *(--s);
        }
    }
    return dest;
}

void *flop_memchr(const void *ptr, int value, size_t num) {
    const uint8_t *p = ptr;
    while (num--) {
        if (*p == (uint8_t)value) {
            return (void *)p;
        }
        p++;
    }
    return NULL;
}

// Garbage Collection (Mark Phase)
void gc_mark(void *ptr) {
    if (!ptr) return;

    FlopMemBlockHeader *block = (FlopMemBlockHeader *)((uint8_t *)ptr - HEADER_SIZE);
    if (block->marked) return;  // Already marked

    block->marked = 1;  // Mark as live
}

// Garbage Collection (Sweep Phase)
void gc_sweep() {
    FlopMemBlockHeader *prev = NULL;
    FlopMemBlockHeader *current = free_list;

    while (current) {
        if (!current->marked) {
            current->is_free = 1;
            if (prev) {
                prev->next = current->next;
            } else {
                free_list = current->next;
            }
        } else {
            current->marked = 0;  // Unmark for next collection cycle
            prev = current;
        }
        current = current->next;
    }
}

// Garbage Collection (Mark-and-Sweep)
void gc_collect() {
    // Step 1: Mark all live objects
    for (size_t i = 0; i < gc_root_count; ++i) {
        gc_mark(gc_roots[i]);
    }

    // Step 2: Sweep (free unmarked objects)
    gc_sweep();
}

// Function to add root references for GC
void gc_add_root(void *ptr) {
    gc_roots = (void **)flop_malloc(sizeof(void *) * (gc_root_count + 1));
    gc_roots[gc_root_count++] = ptr;
}
