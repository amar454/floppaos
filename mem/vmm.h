#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "paging.h"
#define KERNEL_VADDR_BASE  0xC0000000
#define USER_VADDR_BASE         0x00000000 
#define USER_SPACE_END     0xBFFFFFFF
#define GNU_INLINE      __attribute__((always_inline))
#define PAGE_ALIGN(addr)   (((addr) + 0xFFF) & ~0xFFF)
#define RESOLVE_RECURSIVE_PD      ((pde_t*)0xFFFFF000)
#define RESOLVE_RECURSIVE_PT(i)   ((pte_t*)(0xFFC00000 + ((i) << 12)))

int map_page(uintptr_t vaddr, uintptr_t paddr, page_attrs_t attrs);
int unmap_page(uintptr_t vaddr);
int map_range(uintptr_t vaddr, uintptr_t paddr, size_t size, page_attrs_t attrs);
int unmap_range(uintptr_t vaddr, size_t size);
int vmm_init(void);
uintptr_t virt_to_phys(uintptr_t vaddr);
uintptr_t phys_to_virt(uintptr_t paddr);
int vmm_is_mapped(uintptr_t vaddr);
int vmm_protect(uintptr_t vaddr, page_attrs_t attrs);
void vmm_dump_mappings(void);
uintptr_t vmm_find_free_region(uintptr_t min, size_t size);
void* vmm_create_address_space(size_t size, page_attrs_t attrs);
void vmm_free_address_space(void* base, size_t size);
#endif // VMM_H
