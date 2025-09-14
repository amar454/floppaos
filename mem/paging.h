#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

#define PAGE_SIZE             4096
#define PAGE_TABLE_SIZE       1024
#define PAGE_DIRECTORY_SIZE   1024
#define PAGE_SIZE_SHIFT       12
#define KERNEL_PHYSICAL_START 0x00100000
#define CR0_PG_BIT            0x80000000
#define PAGE_PRESENT    0x1
#define PAGE_RW         0x2
#define PAGE_USER    0x4
#define PAGE_PRESENT 0x1


#define TABLE_BYTES     0x1000
#define PAGE_ENTRIES    1024
#define PAGE_MASK       0xFFFFF000

#define KERNEL_VIRT_BASE 0xC0000000U

void paging_init(void);
 void load_pd(uint32_t *pd);
static inline void invlpg(void *va) {
    asm volatile ("invlpg (%0)" : : "a" (va));
}

#endif