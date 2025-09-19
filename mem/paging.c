#include <stdint.h>

#include "pmm.h"
#include "utils.h"
#include "vmm.h"
#include "paging.h"
#include "../apps/echo.h"
#include "../drivers/vga/vgahandler.h"
#include "../lib/logging.h"

static inline uintptr_t virt_to_phys(void *v) {
    uintptr_t va = (uintptr_t)v;
    if (KERNEL_VIRT_BASE == 0) return va;
    if (va >= KERNEL_VIRT_BASE) return va - KERNEL_VIRT_BASE;
    return va; 
}
static inline void *phys_to_virt(uintptr_t p) {
    if (KERNEL_VIRT_BASE == 0) return (void *)p;
    return (void *)(p + KERNEL_VIRT_BASE);
}

uint32_t *pg_dir  = (uint32_t *)0xFFFFF000;
uint32_t *pg_tbls = (uint32_t *)0xFFC00000;
uint32_t* current_pg_dir = NULL;

#define KERNEL_STACK_PAGING_ADDR 0xFF000000

static inline uint32_t page_dir_index_from_va(uint32_t va) {
    return (va >> 22) & 0x3FF;
}

static inline uint32_t virtual_page_index(uint32_t va) {
    return (va >> 12) & 0x3FF;
}

 void load_pd(uint32_t *pd) {
    __asm__ volatile ("mov %0, %%cr3" :: "r"(pd));
}

static void enable_paging(uint8_t enable_wp, uint8_t enable_pse) {
    uint32_t cr0;
    uint32_t cr4;

    __asm__ volatile(
        "mov %%cr0, %0\n"
        "or %1, %0\n"
        "mov %0, %%cr0"
        : "=r"(cr0) 
        : "r"(enable_wp ? 0x80010000 : 0x80000000) // 0x80010000 -> PG + WP
    );

    if (enable_pse) {
        __asm__ volatile(
            "mov %%cr0, %0\n"
            "or %1, %0\n"
            "mov %0, %%cr0"
            : "=r"(cr0)
            : "r"(enable_wp ? 0x80010000 : 0x80000000)
            : "memory"
        );
        
    }
}

static void zero_area(void *area) {
    flop_memset(area, 0, TABLE_BYTES);
}

void paging_init(void) {
    uintptr_t pd_phys = (uintptr_t)pmm_alloc_page();
    if (!pd_phys) {
        log("pmm_alloc_page failed for pd\n", RED);
        return;
    }
    uint32_t *pd = (uint32_t *)phys_to_virt(pd_phys);
    zero_area(pd);

    uintptr_t pt0_phys = (uintptr_t)pmm_alloc_page();
    if (!pt0_phys) {
        log("pmm_alloc_page failed for pt0\n", RED);
        return;
    }
    uint32_t *pt0 = (uint32_t *)phys_to_virt(pt0_phys);
    zero_area(pt0);

    pd[0] = (uint32_t)(pt0_phys & PAGE_MASK) | PAGE_PRESENT | PAGE_RW;
    for (uint32_t i = 0; i < PAGE_ENTRIES; ++i) {
        pt0[i] = (uint32_t)(((uintptr_t)i * TABLE_BYTES) & PAGE_MASK) | PAGE_PRESENT | PAGE_RW;
    }

    uintptr_t pt1022_phys = (uintptr_t)pmm_alloc_page();
    if (!pt1022_phys) {
        log("pmm_alloc_page failed for pt1022\n", RED);
        return;
    }
    pd[1022] = (uint32_t)(pt1022_phys & PAGE_MASK) | PAGE_PRESENT | PAGE_RW;
    uint32_t *pt1022 = (uint32_t *)phys_to_virt(pt1022_phys);
    zero_area(pt1022);

    pd[1023] = (uint32_t)(pd_phys & PAGE_MASK) | PAGE_PRESENT | PAGE_RW;

    pt1022[1023] = (uint32_t)(pd_phys & PAGE_MASK) | PAGE_PRESENT | PAGE_RW;

    current_pg_dir = pd;

    __asm__ volatile("mov %0, %%cr3" : : "r"(pd));

    enable_paging(0,0);
    log("paging enabled\n", YELLOW);


    uint32_t pt_idx = page_dir_index_from_va(KERNEL_STACK_PAGING_ADDR);

    uintptr_t new_pt_phys = (uintptr_t)pmm_alloc_page();
    if (!new_pt_phys) {
        log("pmm_alloc_page failed for new_pt\n", RED);
        return;
    }

    pg_dir[pt_idx] = (uint32_t)(new_pt_phys & PAGE_MASK) | PAGE_PRESENT | PAGE_RW;

    uint32_t *new_pt_virt = &pg_tbls[pt_idx * PAGE_ENTRIES];
    zero_area(new_pt_virt);

    invlpg((void *)KERNEL_STACK_PAGING_ADDR);

    current_pg_dir = (uint32_t *)pg_dir;
    log("paging init - ok\n", YELLOW);
}
