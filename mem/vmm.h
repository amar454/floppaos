#ifndef VMM_H
#define VMM_H
#include "../task/task_handler.h"
#include "paging.h"



void vmm_init();
void vmm_map_page(PDE *page_directory, uintptr_t virt_addr, uintptr_t phys_addr, PageAttributes attrs);
void vmm_unmap_page(PDE *page_directory, uintptr_t virt_addr);

void *vmm_reserve_region(PDE *page_directory, size_t size);
void vmm_free_region(PDE *page_directory, void *virt_addr, size_t size);
void* vmm_malloc(uint32_t size) ;
void vmm_free(void* virtual_address, uint32_t size);
void vmm_init_task(Task* task);
void vmm_switch_to_task(Task* task);
uintptr_t vmm_virt_to_phys(PDE *page_directory, uintptr_t virt_addr);
void* vmm_map_struct_region(PDE *page_directory, size_t struct_size);
void vmm_unmap_struct_region(PDE *page_directory, void *struct_region, size_t struct_size);
#endif