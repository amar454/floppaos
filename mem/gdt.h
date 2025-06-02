#ifndef GDT_H
#define GDT_H

#include <stdint.h>

// GDT Entry structure
typedef struct {
    uint16_t limit_low;     // Lower 16 bits of the limit
    uint16_t base_low;      // Lower 16 bits of the base
    uint8_t base_middle;    // Next 8 bits of the base
    uint8_t access;         // Access flags
    uint8_t granularity;    // Granularity and limit
    uint8_t base_high;      // Last 8 bits of the base
} __attribute__((packed)) GDTEntry;

// GDT Pointer structure
typedef struct {
    uint16_t limit;         // Limit of the GDT
    uint32_t base;          // Base address of the GDT
} __attribute__((packed)) GDTPointer;

// Function declarations
void gdt_flush(uint32_t gdt_ptr_address);
void set_gdt_entry(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity);
void init_gdt();

#endif
