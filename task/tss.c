#include "tss.h"
#include "../mem/gdt.h"
#include "../mem/utils.h"
#include "../lib/logging.h"

extern void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);

static tss_entry_t tss_entry;

void tss_init(uint32_t idx, uint32_t kss, uint32_t kesp) {
    uint32_t base = (uint32_t) &tss_entry;
    uint32_t limit = base + sizeof(tss_entry_t);

    gdt_set_gate(idx, base, limit, 0x89, 0x00);

    flop_memset(&tss_entry, 0, sizeof(tss_entry));

    tss_entry.ss0 = kss;
    tss_entry.esp0 = kesp;
    tss_entry.cs = 0x0b;
    tss_entry.ss = 0x13;
    tss_entry.ds = 0x13;
    tss_entry.es = 0x13;
    tss_entry.fs = 0x13;
    tss_entry.gs = 0x13;
    tss_entry.iomap_base = sizeof(tss_entry_t);

    __asm__ volatile("ltr %%ax" : : "a"((idx * 8) | 0x0));

    log("tss init - ok\n", GREEN);
}

void tss_set_kernel_stack(uint32_t stack) {
    tss_entry.esp0 = stack;
}
