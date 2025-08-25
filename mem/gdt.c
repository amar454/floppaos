
#include <stdint.h>
#include "gdt.h"
#include "../lib/logging.h"

static const uint64_t our_gdt[] = {
    0x0000000000000000ULL,        
    0x00CF9A000000FFFFULL,
    0x00CF92000000FFFFULL           
};

static const gdtr_t gdt_reg = {
    sizeof(our_gdt) - 1,
    (uint32_t)our_gdt
};

void gdt_init() {
    log("gdt init - start\n", GREEN);
    __asm__ volatile (
        "lgdt %0\n"
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        "jmp $0x08, $.flush\n"
        ".flush:\n"
        :
        : "m"(gdt_reg)
        : "eax"
    );
    log("gdt init - ok\n", GREEN);
}
