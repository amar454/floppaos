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

// Aligned page directory and single page table
pde_t pd[PAGE_DIRECTORY_SIZE] __attribute__((aligned(PAGE_SIZE)));
pte_t first_pt[PAGE_TABLE_SIZE] __attribute__((aligned(PAGE_SIZE)));

static inline void load_pd(void* pd_addr) {
    __asm__ volatile ("mov %0, %%cr3" :: "r" (pd_addr) : "memory");
}

static inline void enable_paging(void) {
    __asm__ volatile (
        "mov %%cr0, %%eax\n\t"
        "or $0x80000000, %%eax\n\t"
        "mov %%eax, %%cr0"
        :
        :
        : "eax", "memory"
    );
}


void clear_pds(void) {
    for (int i = 0; i < PAGE_DIRECTORY_SIZE; i++) {
        ((uint32_t*)pd)[i] = 0x00000002; 
    }
}

void pg_frame_set(uintptr_t addr) {
    uintptr_t aligned_addr = addr & ~(PAGE_SIZE - 1);

    pte_attrs_t pt_attrs = {
        .present = 1,
        .rw = 1,
        .user = 0,
        .write_thru = 0,
        .cache_dis = 0,
        .accessed = 0,
        .dirty = 0,
        .global = 0,
        .available = 0,
        .frame_addr = 0  
    };

    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        pt_attrs.frame_addr = (aligned_addr + (i * PAGE_SIZE)) >> 12;
        SET_PF(&first_pt[i], pt_attrs);
    }

    pde_attrs_t pd_attrs = {
        .present = 1,
        .rw = 1,
        .user = 0,
        .write_thru = 0,
        .cache_dis = 0,
        .accessed = 0,
        .page_size = 0,
        .global = 0,
        .available = 0,
        .table_addr = ((uintptr_t)first_pt) >> 12
    };

    SET_PD(&pd[0], pd_attrs);

    pde_attrs_t selfref_attrs = pd_attrs;
    selfref_attrs.table_addr = ((uintptr_t)pd) >> 12;
    SET_PD(&pd[1023], selfref_attrs);
}

void paging_init() {
    log("clearing pds\n", GREEN);
    clear_pds();

    log("pg_frame_set ...\n", GREEN);
    pg_frame_set(0);
    
    log("loading pd\n", GREEN);
    load_pd((void*)pd);

    log("enabling paging\n", GREEN);
    enable_paging();

    log("paging init - ok\n", GREEN);
}
