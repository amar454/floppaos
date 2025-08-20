#include <stdint.h>
#include "pmm.h"
#include "vmm.h"
#include "paging.h"
#include "utils.h"

#include "../lib/logging.h"

#define PAGE_SIZE 4096
#define RECURSIVE_PDE 1023


static uint32_t ensure_pt(uint32_t vaddr, uint32_t pde_flags){
    uint32_t di = pde_index(vaddr);
    uint32_t *pd = virt_pd();
    if (!(pd[di] & PAGE_PRESENT)){
        uint32_t pt_phys = (uint32_t)pmm_alloc_page();
        flop_memset((void*)pt_phys, 0, PAGE_SIZE);
        pd[di] = (pt_phys & ~0xFFF) | pde_flags | PAGE_PRESENT;
    }
    return di;
}

int vmm_map_page(uint32_t vaddr, uint32_t paddr, uint32_t flags){
    uint32_t di = ensure_pt(vaddr, flags);
    uint32_t table_index = pte_index(vaddr);
    uint32_t *pt = virt_pt(di);
    if (pt[table_index] & PAGE_PRESENT) return -1;
    pt[table_index] = (paddr & ~0xFFF) | (flags | PAGE_PRESENT);
    invlpg((void*)vaddr);
    return 0;
}

int vmm_unmap_page(uint32_t vaddr){
    uint32_t di = pde_index(vaddr);
    uint32_t *pd = virt_pd();
    if (!(pd[di] & PAGE_PRESENT)) return -1;
    uint32_t table_index = pte_index(vaddr);
    uint32_t *pt = virt_pt(di);
    if (!(pt[table_index] & PAGE_PRESENT)) return -1;
    pt[table_index] = 0;
    invlpg((void*)vaddr);
    return 0;
}

uint32_t vmm_resolve(uint32_t vaddr){
    uint32_t di = pde_index(vaddr);
    uint32_t *pd = virt_pd();
    if (!(pd[di] & PAGE_PRESENT)) return 0;
    uint32_t table_index = pte_index(vaddr);
    uint32_t *pt = virt_pt(di);
    if (!(pt[table_index] & PAGE_PRESENT)) return 0;
    return (pt[table_index] & ~0xFFF) | (vaddr & 0xFFF);
}



extern uint32_t *pg_dir;
extern uint32_t *pg_tbls;
extern uint32_t* current_pg_dir;

void vmm_init(void) {
    uint32_t *pd = current_pg_dir; 

    // map kernel 1 GiB: VA 0xC0000000 → PA 0x00000000..0x3FFFFFFF
    for (uint32_t va = KERNEL_BASE; va < 0xFFFFFFFF; va += PAGE_SIZE) {
        uint32_t paddr = va - KERNEL_BASE;
        uint32_t pg_dir_index = (va >> 22) & 0x3FF;
        uint32_t pg_tbl_index = (va >> 12) & 0x3FF;

        if (!(pd[pg_dir_index] & PAGE_PRESENT)) {
            uint32_t *pt = (uint32_t*)pmm_alloc_page();
            flop_memset(pt, 0, PAGE_SIZE);
            pd[pg_dir_index] = ((uint32_t)pt & ~0xFFF) | PAGE_PRESENT | PAGE_RW;
        }

        uint32_t *pt_va = (uint32_t*)(0xFFC00000 + (pg_dir_index << 12));
        pt_va[pg_tbl_index] = (paddr & ~0xFFF) | PAGE_PRESENT | PAGE_RW;
    }

    log("vmm init - ok", GREEN);
}
