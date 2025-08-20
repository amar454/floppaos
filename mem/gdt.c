#include <stdint.h>
#include <string.h>
#include "../lib/logging.h"
#include "gdt.h"
#include "utils.h"


extern uint8_t stack_space;
#define KSTACKTOP ((uint32_t)(stack_space + 8192))



static tss_entry_t ktss;

void tss_init() {
    flop_memset(&ktss, 0, sizeof(ktss));

    ktss.ss0  = 0x10;       // kernel data segment
    ktss.esp0 = KSTACKTOP;  // top of kernel stack
    ktss.cs   = 0x0b;       // user code segment
    ktss.ds   = 0x13;       // user data segment
    ktss.es   = 0x13;
    ktss.fs   = 0x13;
    ktss.gs   = 0x13;
    ktss.ss   = 0x13;
    ktss.iomap = sizeof(tss_entry_t);
}

static uint64_t our_gdt[6] = {
    0x0000000000000000ULL,
    0x00CF9A000000FFFFULL, 
    0x00CF92000000FFFFULL, 
    0x00CFFA000000FFFFULL, 
    0x00CFF2000000FFFFULL, 
    0                        
};

static const gdtr_t gdt_reg = {
    sizeof(our_gdt) - 1,
    (uint32_t)our_gdt
};

void gdt_init(void) {
    log("gdt init - start\n", GREEN);

    tss_init(); 

    uint32_t tss_base = (uint32_t)&ktss;
    uint32_t tss_limit = sizeof(ktss) - 1;

    our_gdt[5] =
        ((uint64_t)(tss_base & 0xFFFF) |
        ((uint64_t)(tss_limit & 0xFFFF) << 16) |
        ((uint64_t)(((tss_base >> 16) & 0xFF) | (0x89 << 8) | ((tss_limit >> 16) & 0x0F)) << 32) |
        ((uint64_t)((tss_base >> 24) & 0xFF) << 56));

    __asm__ volatile (
        "lgdt %[gdtr]\n"
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        "jmp $0x08, $flush\n"
        "flush:\n"
        :
        : [gdtr] "m"(gdt_reg)
        : "eax"
    );

    log("gdt init - ok\n", GREEN);
}
