#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include "../multiboot/multiboot.h"
#include "paging.h"
#define PAGE_SIZE 4096
#define PAGE_SHIFT 12
#define MAX_ORDER 10
// minimum kernel heap size

#define PAGE_SIZE 4096

// page info struct
struct Page {
    uintptr_t address;
    uint32_t order;
    int is_free;
    struct Page* next;
};

// buddy allocator struct
struct BuddyAllocator {
    struct Page* free_list[MAX_ORDER + 1];
    struct Page* page_info; // Tracks all pages
    uint32_t total_pages;
    uintptr_t memory_start;
    uintptr_t memory_end;
};

// buddy allocator struct
extern struct BuddyAllocator pmm_buddy;

// Function prototypes
void pmm_init(multiboot_info_t* mb_info);
void* pmm_alloc_pages(uint32_t order, uint32_t count);
void* pmm_alloc_page(void);
void pmm_free_pages(void* addr, uint32_t order, uint32_t count) ;
void pmm_free_page(void* addr);
void buddy_init(uintptr_t start, uint32_t total_pages);
void buddy_split(uintptr_t addr, uint32_t order);
void buddy_merge(uintptr_t addr, uint32_t order);
int pmm_get_memory_size();
struct Page* phys_to_page_index(uintptr_t addr);
uint32_t page_index(uintptr_t addr);



#endif // PMM_H
