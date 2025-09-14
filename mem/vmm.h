#ifndef VMM_H
#define VMM_H

#include <stdint.h>

#define PAGE_SIZE     4096
#define RECURSIVE_PDE 1023

typedef struct vmm_region {
    uint32_t *pg_dir;
    struct vmm_region *next;
} vmm_region_t;

extern uint32_t *pg_dir;
extern uint32_t *pg_tbls;
extern uint32_t *current_pg_dir;

int vmm_alloc_pde(uint32_t *dir, uint32_t pde_idx, uint32_t flags);

int vmm_map(vmm_region_t *region, uintptr_t va, uintptr_t pa, uint32_t flags);
int vmm_unmap(vmm_region_t *region, uintptr_t va);
uintptr_t vmm_resolve(vmm_region_t *region, uintptr_t va);

vmm_region_t *vmm_region_create(void);
void vmm_region_destroy(vmm_region_t *region);

vmm_region_t *vmm_copy_pagemap(vmm_region_t *src);
void vmm_nuke_pagemap(vmm_region_t *region);

void vmm_switch(vmm_region_t *region);
void vmm_init(void);

#endif
