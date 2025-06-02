#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

#define PAGE_DIRECTORY_SIZE 1024
#define PAGE_TABLE_SIZE     1024
#define PAGE_SIZE           4096

// Page Table Entry (PTE) structure
typedef struct {
    uint32_t present    : 1;
    uint32_t rw         : 1;
    uint32_t user       : 1;
    uint32_t write_thru : 1;
    uint32_t cache_dis  : 1;
    uint32_t accessed   : 1;
    uint32_t dirty      : 1;
    uint32_t pat        : 1;
    uint32_t global     : 1;
    uint32_t available  : 3;
    uint32_t frame_addr : 20;
} __attribute__((packed)) PTE;

// Page Directory Entry (PDE) structure
typedef struct {
    uint32_t present    : 1;
    uint32_t rw         : 1;
    uint32_t user       : 1;
    uint32_t write_thru : 1;
    uint32_t cache_dis  : 1;
    uint32_t accessed   : 1;
    uint32_t reserved   : 1;
    uint32_t page_size  : 1;
    uint32_t global     : 1;
    uint32_t available  : 3;
    uint32_t table_addr : 20;
} __attribute__((packed)) PDE;

// Helper structure for page attributes
typedef struct {
    uint32_t present    : 1;
    uint32_t rw         : 1;
    uint32_t user       : 1;
    uint32_t write_thru : 1;
    uint32_t cache_dis  : 1;
    uint32_t accessed   : 1;
    uint32_t dirty      : 1;
    uint32_t pat        : 1;
    uint32_t global     : 1;
    uint32_t available  : 3;
    uint32_t frame_addr : 20;
} PageAttributes;

// Global page directory and page tables
extern PDE page_directory[PAGE_DIRECTORY_SIZE];
extern PTE page_tables[PAGE_DIRECTORY_SIZE][PAGE_TABLE_SIZE];

// Function declarations
void set_page_flags(PTE *entry, PageAttributes attributes);
void enable_paging(uint8_t enable_wp, uint8_t enable_pse);

PDE* create_page_directory();
PTE* create_page_table();
PTE* get_page_table(PDE *page_directory, uint32_t index);
PTE* get_page_table_entry(PDE *page_directory, uintptr_t virt_addr);
PTE* get_page_table_entry_by_index(PDE *page_directory, uint32_t index);
PTE* destroy_page_table(PDE *page_directory, uint32_t index);
void destroy_page_directory(PDE *page_directory);

PDE* create_user_page_directory();
PDE* destroy_user_page_directory(PDE *user_page_directory);

void paging_init();

#endif