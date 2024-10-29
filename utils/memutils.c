#include "memutils.h"
#include <stdint.h>
#include <stddef.h>

#define SIMULATED_DISK_SIZE 1024 * 1024 // Size of the simulated memory pool

static uint8_t simulated_memory[SIMULATED_DISK_SIZE]; // Simulated memory pool
static uint32_t next_free_offset = 0; // Next free position in the simulated memory

// Custom flop equivalent for malloc
void *flop_malloc(size_t size) {
    if (next_free_offset + size > SIMULATED_DISK_SIZE) {
        return NULL; // Not enough memory
    }
    void *ptr = &simulated_memory[next_free_offset];
    next_free_offset += size; // Move the free offset forward
    return ptr;
}

// Custom flop equivalent for free
void flop_free(void *ptr) {
    // TBA
}

// Custom flop equivalent for memcpy
void *flop_memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}
