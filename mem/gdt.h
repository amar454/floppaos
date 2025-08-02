#ifndef GDT_H
#define GDT_H

#include <stdint.h>

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed)) GDTEntry;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) GDTPointer;

void gdt_flush(uint32_t gdt_ptr_address);
void set_gdt_entry(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity);
void init_gdt();

#endif
