#include <stdint.h>

#include "pmm.h"
#include "utils.h"
#include "vmm.h"
#include "paging.h"
#include "../apps/echo.h"
#include "../drivers/vga/vgahandler.h"
#include "../lib/logging.h"

#define TABLE_BYTES     0x1000
#define PAGE_ENTRIES    1024
#define PAGE_MASK       0xFFFFF000


uint32_t *pg_dir  = (uint32_t *)0xFFFFF000; 
uint32_t *pg_tbls = (uint32_t *)0xFFC00000;
uint32_t* current_pg_dir = NULL;

#define KERNEL_STACK_PAGING_ADDR 0xFF000000  /* kept from your previous code */

static inline uint32_t page_dir_index_from_va(uint32_t va) {
    return (va >> 22) & 0x3FF;
}
static inline uint32_t virtual_page_index(uint32_t va) {
    return (va >> 12) & 0x3FF;
}

void load_pd(uintptr_t pd) {
    __asm__ volatile ("mov %0, %%cr3" :: "r"(pd) : "memory");
}
 void enable_paging(void) {
    uint32_t cr0;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; 
    __asm__ volatile ("mov %0, %%cr0" :: "r"(cr0));
}

static void zero_area(void *area) {
    flop_memset(area, 0, TABLE_BYTES);
}

void paging_init(void) {
    log("creating page directory\n", YELLOW);

    uint32_t *pd = (uint32_t *)pmm_alloc_page();
    zero_area(pd);

    /* Identity-map first 4 MiB via pt0 */
    uint32_t *pt0 = (uint32_t *)pmm_alloc_page();
    zero_area(pt0);

    pd[0] = (uint32_t)pt0 | PAGE_PRESENT | PAGE_RW;
    for (uint32_t i = 0; i < PAGE_ENTRIES; ++i) {
        pt0[i] = (i * TABLE_BYTES) | PAGE_PRESENT | PAGE_RW;
    }

    log("creating second-last table (index 1022)\n", YELLOW);
    pd[1022] = (uint32_t)pmm_alloc_page() | PAGE_PRESENT | PAGE_RW;
    uint32_t *pt1022 = (uint32_t *)(pd[1022] & PAGE_MASK);
    zero_area(pt1022);

    pt1022[1023] = (uint32_t)pd | PAGE_PRESENT | PAGE_RW;

    pd[1023] = (uint32_t)pd | PAGE_PRESENT | PAGE_RW;

    current_pg_dir = pd;

    log("loading pd...\n", YELLOW);
    load_pd((uintptr_t)pd);

    log("enabling paging\n", YELLOW);
    enable_paging();

 
    uint32_t pt_idx = page_dir_index_from_va(KERNEL_STACK_PAGING_ADDR);
    pg_dir[pt_idx] = (uint32_t)pmm_alloc_page() | PAGE_PRESENT | PAGE_RW;

    zero_area(&pg_tbls[pt_idx * PAGE_ENTRIES]);

    current_pg_dir = pd;
    log("paging init... ok", YELLOW);
}
