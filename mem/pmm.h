#ifndef PMM_H
#define PMM_H
#include <stdint.h>
#include "../multiboot/multiboot.h"
#define PAGE_SIZE 4096
#define MAX_ORDER 10

struct Page {
    uintptr_t address;
    int is_free;
    uint32_t order;
    struct Page* next;
};

struct PageInfo {
    uint32_t page;
    int is_free;
};

struct PageCache {
    struct PageInfo* pages;
    uint32_t capacity;
    uint32_t count;
};

struct BuddyAllocator {
    struct Page* free_list[MAX_ORDER + 1];
    uintptr_t memory_start;
    uintptr_t memory_end;
    uint32_t total_pages;
    struct PageCache cache;
};

extern struct BuddyAllocator pmm_buddy;

void pmm_init(multiboot_info_t* mb_info);
void* pmm_alloc_pages(uint32_t order);
void* pmm_alloc_page(void);
void pmm_free_pages(void* addr, uint32_t order);
void pmm_free_page(void* addr);

void buddy_init(uintptr_t start, uint32_t total_pages);
void buddy_split(uintptr_t addr, uint32_t order);
void buddy_merge(uintptr_t addr, uint32_t order);

#endif // PMM_H