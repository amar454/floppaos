#ifndef VMM_H
#define VMM_H

#include <stdint.h>

#define PAGE_SIZE     4096
#define RECURSIVE_PDE 1023

typedef struct aslr_entry {
    uintptr_t va;
    size_t pages;
    size_t align;
    uint32_t flags;
} aslr_entry_t;

typedef struct vmm_region {
    uint32_t *pg_dir;
    struct vmm_region *next;
    aslr_entry_t *random_table;
    size_t random_count;
    size_t random_capacity;
} vmm_region_t;

typedef struct {
    uintptr_t va;      
    uintptr_t pa;       
} flop_rand_entry_t;

typedef struct {
    vmm_region_t *src_region;      
    vmm_region_t *table_region;  
    flop_rand_entry_t *entries;  
    size_t page_count;            
} flop_randframe_region_t;

extern uint32_t *pg_dir;
extern uint32_t *pg_tbls;
extern uint32_t *current_pg_dir;
int vmm_alloc_pde(uint32_t *dir, uint32_t pde_idx, uint32_t flags);
#define USER_SPACE_START 0x00100000U
#define USER_SPACE_END   0xBFFFFFFFU
uintptr_t vmm_aslr_alloc(vmm_region_t *region, size_t pages, size_t align, uint32_t flags);
void vmm_aslr_free(vmm_region_t *region, uintptr_t va);

uintptr_t vmm_resolve(vmm_region_t *region, uintptr_t va);
int vmm_map(vmm_region_t *region, uintptr_t va, uintptr_t pa, uint32_t flags);
int vmm_unmap(vmm_region_t *region, uintptr_t va);
int vmm_map_range(vmm_region_t *region, uintptr_t va, uintptr_t pa, size_t pages, uint32_t flags);
int vmm_unmap_range(vmm_region_t *region, uintptr_t va, size_t pages);
uintptr_t vmm_find_free_range(vmm_region_t *region, size_t pages);
int vmm_protect(vmm_region_t *region, uintptr_t va, uint32_t flags);
vmm_region_t *vmm_region_create();
void vmm_region_destroy(vmm_region_t *region);
void vmm_switch(vmm_region_t *region);

void vmm_init();
#endif
