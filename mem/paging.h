#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

#define PAGE_DIRECTORY_SIZE 1024
#define PAGE_TABLE_SIZE     1024
#define PAGE_SIZE           4096

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
    uint32_t present : 1;
    uint32_t rw : 1;
    uint32_t user : 1;
    uint32_t write_thru : 1;
    uint32_t cache_dis : 1;
    uint32_t accessed : 1;
    uint32_t dirty : 1;
    uint32_t pat : 1;
    uint32_t global : 1;
    uint32_t available : 3;
    uint32_t frame_addr : 20;
} PageAttributes;

extern PDE page_directory[PAGE_DIRECTORY_SIZE];
extern PTE page_tables[PAGE_DIRECTORY_SIZE][PAGE_TABLE_SIZE];
void set_page_flags(PTE *entry, PageAttributes attributes);
void enable_paging(uint8_t enable_wp, uint8_t enable_pse);
void paging_init();

#endif
