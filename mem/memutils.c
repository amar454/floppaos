#include "memutils.h"
#include <stdint.h>
#include <stddef.h>
#include "../apps/echo.h"
#include "../drivers/vga/vgacolors.h"
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
void *flop_memset(void *dest, int value, size_t size) {
    uint8_t val = (uint8_t)value;  // Value to set in memory

    asm volatile (
        "cld\n\t"          // Clear direction flag (forward direction)
        "rep stosb"        // Repeat storing AL into memory
        : "+D" (dest), "+c" (size) // Outputs: Update EDI (dest) and ECX (size)
        : "a" (val)         // Inputs: AL (value to set)
        : "memory"          // Clobber: Memory is modified
    );

    return dest;
}
int flop_memcmp(const void *ptr1, const void *ptr2, size_t num) {
    if (!ptr1 || !ptr2) {
        echo("flop_memcmp: NULL pointer detected!\n", RED);
        return -1;  // Convention: return -1 to indicate error
    }

    const uint8_t *p1 = (const uint8_t*)ptr1;
    const uint8_t *p2 = (const uint8_t*)ptr2;

    for (size_t i = 0; i < num; i++) {
        if (p1[i] != p2[i]) {
            return (p1[i] - p2[i]);  // Return the difference of the first non-matching byte
        }
    }
    return 0;  // Memory regions are equal
}
void *flop_memcpy(void *dest, const void *src, size_t n) {
    if (!dest || !src) {
        echo("flop_memcpy: NULL pointer detected!\n", RED);
        return NULL;  // Return NULL to indicate an error
    }

    unsigned char *d = dest;
    const unsigned char *s = src;

    // Check for overlap (undefined behavior)
    if ((d > s && d < s + n) || (s > d && s < d + n)) {
        echo("flop_memcpy: Overlapping regions detected! Use memmove instead.\n", YELLOW);
        return NULL;  // Indicate an error
    }

    while (n--) {
        *d++ = *s++;
    }

    return dest;
}

