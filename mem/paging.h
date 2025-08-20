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

#define KERNEL_BASE 0xC0000000
#define KERNEL_SIZE 0x40000000  // 1 GiB

void paging_init(void);


#endif