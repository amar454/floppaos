#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include "../multiboot/multiboot.h"
#include "paging.h"
#include "../task/sync/spinlock.h"
#define PAGE_SIZE 4096
#define PAGE_SHIFT 12
#define MAX_ORDER 10

#define PAGE_SIZE 4096

struct Page {
    uintptr_t address;
    uint32_t order;
    int is_free;
    struct Page* next;
};

struct buddy_allocator_t {
    struct Page* free_list[MAX_ORDER + 1];
    struct Page* page_info;
    uint32_t total_pages;
    uintptr_t memory_start;
    uintptr_t memory_end;
    uint32_t memory_base;
    spinlock_t lock;
};

typedef struct page_cache_entry {
    uintptr_t phys;
    uint64_t idx;
    struct page_cache_entry *prev_lru;
    struct page_cache_entry *next_lru;
    bool dirty;
    uint32_t refcount;
} page_cache_entry_t;

typedef struct {
    uint8_t *keys;
    void   **vals;
    uint8_t *state;  /* 0=empty,1=used,2=deleted */
    size_t   cap;
    size_t   len;
} child_map_t;

typedef struct radix_node {
    page_cache_entry_t *entry;
    child_map_t map;
} radix_node_t;

typedef struct {
    radix_node_t *root;
} radix_tree_t;

typedef struct {
    radix_tree_t *tree;
    page_cache_entry_t *lru_head;
    page_cache_entry_t *lru_tail;
    spinlock_t lock;
    uint64_t page_count;
} page_cache_t;
extern uint32_t *pg_dir;
extern uint32_t *pg_tbls;
extern page_cache_t page_cache;
extern struct buddy_allocator_t buddy;

void pmm_init(multiboot_info_t* mb_info);
void* pmm_alloc_pages(uint32_t order, uint32_t count);
void* pmm_alloc_page(void);
void pmm_free_pages(void* addr, uint32_t order, uint32_t count) ;
void pmm_free_page(void* addr);
void buddy_split(uintptr_t addr, uint32_t order);
void buddy_merge(uintptr_t addr, uint32_t order);
uint32_t pmm_get_memory_size();
uint32_t pmm_get_page_count();
struct Page* phys_to_page_index(uintptr_t addr);
uint32_t page_index(uintptr_t addr);
void pmm_copy_page(void* dst, void* src);
int pmm_is_valid_addr(uintptr_t addr);
#endif
