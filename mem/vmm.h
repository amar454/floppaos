#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#include <stddef.h>
#include "paging.h"

// Virtual memory region structure
typedef struct {
    uintptr_t start_virt; 
    uintptr_t start_phys; 
    size_t size;         
    uint32_t pages;       
    PageAttributes attrs; 
} vmm_region;
void vmm_destroy_region(PDE *page_directory, vmm_region region);
void vmm_init();
void vmm_map_page(PDE *page_directory, uintptr_t virt_addr, uintptr_t phys_addr, PageAttributes attrs);
void vmm_unmap_page(PDE *page_directory, uintptr_t virt_addr);
vmm_region vmm_create_region(PDE *page_directory, uintptr_t start_virt, uintptr_t start_phys, size_t size, PageAttributes attrs);
void* vmm_malloc(uint32_t size);
void vmm_free(void* virtual_address, uint32_t size);
void test_vmm();
uintptr_t vmm_virt_to_phys(PDE *page_directory, uintptr_t virt_addr);
void* vmm_map_struct_region(PDE *page_directory, size_t struct_size);
void vmm_unmap_struct_region(PDE *page_directory, void *struct_region, size_t struct_size);

#endif // VMM_H