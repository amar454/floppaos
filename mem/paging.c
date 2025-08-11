#include <stdint.h>

#include "pmm.h"
#include "utils.h"
#include "vmm.h"
#include "paging.h"
#include "../apps/echo.h"
#include "../drivers/vga/vgahandler.h"
#include "../lib/logging.h"

#define PAGE_DIRECTORY_SIZE 1024
#define PAGE_TABLE_SIZE 1024
#define PAGE_SIZE 4096

pde_t pd[PAGE_DIRECTORY_SIZE] __attribute__((aligned(PAGE_SIZE)));
pte_t first_pt[PAGE_TABLE_SIZE] __attribute__((aligned(PAGE_SIZE)));
 
static void load_pd(void* pd_addr) {
    __asm__ volatile ("mov %0, %%cr3" :: "r" (pd_addr) : "memory");
}

static void enable_paging(void) {
    uint32_t cr0;
    __asm__ volatile (
        "mov %%cr0, %0" : "=r"(cr0)
    );
    cr0 |= CR0_PG_BIT;
    __asm__ (
        "mov %0, %%cr0" 
        :: "r"(cr0)
    );
}

pde_t *current_page_directory = NULL;

void paging_init(void) {
    void *pd_page = pmm_alloc_page();
    void *pt_page = pmm_alloc_page();

    if (!pd_page || !pt_page) {
        log("paging_init: pmm_alloc_page failed\n", RED);
        return;
    }
    
    pde_t *pd = (pde_t *)pd_page;
    pte_t *first_pt = (pte_t *)pt_page;

    flop_memset(pd, 0, PAGE_SIZE);
    flop_memset(first_pt, 0, PAGE_SIZE);

    // Identity map first 4 MiB (4KiB pages)
    for (uint32_t i = 0; i < PAGE_TABLE_SIZE; ++i) {
        pte_attrs_t a = {0};
        a.present    = 1;
        a.rw         = 1;
        a.frame_addr = i;
        SET_PF(&first_pt[i], a);
    }

    pde_attrs_t pd0 = {0};
    pd0.present    = 1;
    pd0.rw         = 1;
    pd0.table_addr = ((uint32_t)first_pt) >> PAGE_SIZE_SHIFT;
    SET_PD(&pd[0], pd0);

    /* 
    * Recursive mapping:
    *
    * PDE 1023 points to the Page Directory itself:
    *
    *   +-------------------------+ 0xFFFFF000
    *   | Page Directory (PD)     | <--- PDE[1023] points here 
    *   +-------------------------+
    *              ^
    *              ^
    *   +-------------------------+ 0xFFC00000
    *   | Page Tables (PTs) area  |  Covers PDE[0..1022]
    *   | +---------------------+ |
    *   | | PT 0                | | <-- PDE[0]
    *   | +---------------------+ |
    *   | | PT 1                | | <-- PDE[1]
    *   | +---------------------+ |
    *   | |        ...          | | <-- PDE[...]
    *   | +---------------------+ |
    *   | | PT 1022             | | <-- PDE[1022]
    *   | +---------------------+ |
    *   +-------------------------+
    *
    * virt address bit layout (32 bits):
    *
    *   [10 bits PDE][10 bits PT][12 bits offset]
    *
    * get pt:
    *
    *   0xFFC00000 + (PDE_index << 12) + (PT_index * 4)
    *
    * get pd:
    *
    *   0xFFFFF000 + (PDE_index * 4)
    */

    pde_attrs_t rec = {0};
    rec.present    = 1;
    rec.rw         = 1; 
    rec.table_addr = ((uint32_t)pd) >> PAGE_SIZE_SHIFT;
    SET_PD(&pd[1023], rec);

    load_pd(pd);

    current_page_directory = pd;

    log("enabling paging\n", GREEN);
    enable_paging();
    log("paging init - ok\n", GREEN);

}