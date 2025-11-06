#ifndef GDT_H
#define GDT_H

#include <stdint.h>

typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint32_t base;
} gdtr_t;

void gdt_init(void);
void gdt_set_gate(int idx, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);

#endif
