#ifndef VMM_H
#define VMM_H
#include "../task/task_handler.h"
#include "paging.h"
#define KERNEL_PAGE_START 768 // Kernel space starts at the 3GB mark
#define TASK_STACK_START 0xBFFFF000 // Task stack virtual address (below 3GB)


void vmm_init();
void vmm_map_page(PDE *page_directory, uintptr_t virt_addr, uintptr_t phys_addr, PageAttributes attrs);
void vmm_init_task(Task *task);
void* vmm_malloc(uint32_t size) ;
void vmm_free(void* virtual_address, uint32_t size);
void vmm_init_task(Task* task);
void vmm_switch_to_task(Task* task);
uintptr_t vmm_get_physical_address(PDE *page_directory, uintptr_t virt_addr);
//extern uint32_t page_directory[PAGE_DIRECTORY_SIZE] __attribute__((aligned(PAGE_SIZE)));
//extern uint32_t first_page_table[PAGE_TABLE_SIZE] __attribute__((aligned(PAGE_SIZE)));
#endif