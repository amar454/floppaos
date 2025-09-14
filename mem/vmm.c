#include <stdint.h>
#include "pmm.h"
#include "vmm.h"
#include "paging.h"
#include "utils.h"
#include "../cpu/cpu.h"
#include "../lib/logging.h"

#define PAGE_SIZE 4096
#define RECURSIVE_PDE 1023


extern uint32_t *pg_dir;
extern uint32_t *pg_tbls;
extern uint32_t *current_pg_dir;

static vmm_region_t *region_list = 0;
static vmm_region_t kernel_region;

static inline uint32_t pd_index(uintptr_t va) {
    return (va >> 22) & 0x3FF;
}

static inline uint32_t pt_index(uintptr_t va) {
    return (va >> 12) & 0x3FF;
}

static inline uint32_t page_offset(uintptr_t va) {
    return va & 0xFFF;
}

int vmm_alloc_pde(uint32_t *dir, uint32_t pde_idx, uint32_t flags) {
    if (dir[pde_idx] & PAGE_PRESENT) return 0;
    uintptr_t pt_phys = (uintptr_t)pmm_alloc_page();
    if (!pt_phys) return -1;
    dir[pde_idx] = (pt_phys & PAGE_MASK) | flags | PAGE_PRESENT;
    flop_memset(&pg_tbls[pde_idx * PAGE_ENTRIES], 0, PAGE_SIZE);
    return 0;
}

int vmm_map(vmm_region_t *region, uintptr_t va, uintptr_t pa, uint32_t flags) {
    uint32_t pdi = pd_index(va);
    uint32_t pti = pt_index(va);
    if (vmm_alloc_pde(region->pg_dir, pdi, flags) < 0) return -1;
    uint32_t *pt = &pg_tbls[pdi * PAGE_ENTRIES];
    pt[pti] = (pa & PAGE_MASK) | flags | PAGE_PRESENT;
    invlpg((void *)va);
    return 0;
}

int vmm_unmap(vmm_region_t *region, uintptr_t va) {
    uint32_t pdi = pd_index(va);
    uint32_t pti = pt_index(va);
    if (!(region->pg_dir[pdi] & PAGE_PRESENT)) return -1;
    uint32_t *pt = &pg_tbls[pdi * PAGE_ENTRIES];
    pt[pti] = 0;
    invlpg((void *)va);
    return 0;
}

uintptr_t vmm_resolve(vmm_region_t *region, uintptr_t va) {
    uint32_t pdi = pd_index(va);
    uint32_t pti = pt_index(va);
    if (!(region->pg_dir[pdi] & PAGE_PRESENT)) return 0;
    uint32_t *pt = &pg_tbls[pdi * PAGE_ENTRIES];
    if (!(pt[pti] & PAGE_PRESENT)) return 0;
    return (pt[pti] & PAGE_MASK) | (va & ~PAGE_MASK);
}

static void region_insert(vmm_region_t *region) {
    region->next = region_list;
    region_list = region;
}

static void region_remove(vmm_region_t *region) {
    vmm_region_t **iter = &region_list;
    while (*iter) {
        if (*iter == region) {
            *iter = region->next;
            break;
        }
        iter = &(*iter)->next;
    }
}

vmm_region_t *vmm_region_create() {
    uintptr_t dir_phys = (uintptr_t)pmm_alloc_page();
    if (!dir_phys) return 0;
    uint32_t *dir = (uint32_t *)dir_phys;
    flop_memset(dir, 0, PAGE_SIZE);
    dir[RECURSIVE_PDE] = (dir_phys & PAGE_MASK) | PAGE_PRESENT | PAGE_RW;
    vmm_region_t *region = (vmm_region_t *)kmalloc(sizeof(vmm_region_t));
    region->pg_dir = dir;
    region->next = 0;
    region_insert(region);
    return region;
}

void vmm_region_destroy(vmm_region_t *region) {
    region_remove(region);
    pmm_free_page(region->pg_dir);
    kfree(region, sizeof(vmm_region_t));
}

void vmm_switch(vmm_region_t *region) {
    current_pg_dir = region->pg_dir;
    load_pd(region->pg_dir);
}

void vmm_init() {
    kernel_region.pg_dir = pg_dir;
    kernel_region.next = 0;
    current_pg_dir = pg_dir;
    pg_dir[RECURSIVE_PDE] = ((uintptr_t)pg_dir & PAGE_MASK) | PAGE_PRESENT | PAGE_RW;
    region_insert(&kernel_region);
    // we do not need to load the kernel region because it has
    // been loaded into cr3 by the paging init function.
    log("vmm init - ok", GREEN);
}

vmm_region_t *vmm_copy_pagemap(vmm_region_t *src) {
    uintptr_t new_dir_phys = (uintptr_t)pmm_alloc_page();
    if (!new_dir_phys) return 0;
    uint32_t *new_dir = (uint32_t *)new_dir_phys;
    flop_memset(new_dir, 0, PAGE_SIZE);

    vmm_region_t *dst = (vmm_region_t *)kmalloc(sizeof(vmm_region_t));
    dst->pg_dir = new_dir;
    dst->next = 0;

    for (int pdi = 0; pdi < 1024; pdi++) {
        if (!(src->pg_dir[pdi] & PAGE_PRESENT)) continue;
        uintptr_t pt_phys = (uintptr_t)pmm_alloc_page();
        if (!pt_phys) { 
            vmm_region_destroy(dst); 
            return 0; 
        }
        uint32_t *src_pt = &pg_tbls[pdi * PAGE_ENTRIES];
        uint32_t *dst_pt = (uint32_t *)pt_phys;
        flop_memset(dst_pt, 0, PAGE_SIZE);
        for (int pti = 0; pti < PAGE_ENTRIES; pti++) {
            if (!(src_pt[pti] & PAGE_PRESENT)) continue;
            uintptr_t new_page = (uintptr_t)pmm_alloc_page();
            if (!new_page) { 
                vmm_region_destroy(dst); 
                return 0;
            }
            flop_memcpy((void *)new_page, (void *)(src_pt[pti] & PAGE_MASK), PAGE_SIZE);
            dst_pt[pti] = (new_page & PAGE_MASK) | (src_pt[pti] & ~PAGE_MASK);
        }

        new_dir[pdi] = (pt_phys & PAGE_MASK) | (src->pg_dir[pdi] & ~PAGE_MASK);
    }
    new_dir[RECURSIVE_PDE] = (new_dir_phys & PAGE_MASK) | PAGE_PRESENT | PAGE_RW;
    region_insert(dst);
    return dst;
}

void vmm_nuke_pagemap(vmm_region_t *region) {
    for (int pdi = 0; pdi < 1024; pdi++) {
        if (!(region->pg_dir[pdi] & PAGE_PRESENT)) continue;

        uint32_t *pt = &pg_tbls[pdi * PAGE_ENTRIES];
        for (int pti = 0; pti < PAGE_ENTRIES; pti++) {
            if (pt[pti] & PAGE_PRESENT) {
                uintptr_t pa = pt[pti] & PAGE_MASK;
                pmm_free_page((void *)pa);
            }
        }

        uintptr_t pt_phys = region->pg_dir[pdi] & PAGE_MASK;
        pmm_free_page((void *)pt_phys);
    }

    uintptr_t dir_phys = (uintptr_t)region->pg_dir;
    pmm_free_page((void *)dir_phys);

    region_remove(region);
    kfree(region, sizeof(vmm_region_t));
}
