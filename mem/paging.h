#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

#define PAGE_SIZE             4096
#define PAGE_TABLE_SIZE       1024
#define PAGE_DIRECTORY_SIZE   1024
#define PAGE_SIZE_SHIFT       12
#define K_PD_INDEX            768
#define K_HEAP_PD_INDEX       769
#define K_HEAP_START          0x00400000
#define KERNEL_PHYSICAL_START 0x00100000
#define CR0_PG_BIT            0x80000000
#define PAGE_PRESENT    0x1
#define PAGE_RW         0x2
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
} __attribute__((packed)) pte_t;

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
} __attribute__((packed)) pde_t;

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
} page_attrs_t;

#define SET_PF(entry, attrs) do {             \
    (entry)->present    = (attrs).present;    \
    (entry)->rw         = (attrs).rw;         \
    (entry)->user       = (attrs).user;       \
    (entry)->write_thru = (attrs).write_thru; \
    (entry)->cache_dis  = (attrs).cache_dis;  \
    (entry)->accessed   = (attrs).accessed;   \
    (entry)->dirty      = (attrs).dirty;      \
    (entry)->pat        = (attrs).pat;        \
    (entry)->global     = (attrs).global;     \
    (entry)->available  = (attrs).available;  \
    (entry)->frame_addr = (attrs).frame_addr; \
} while(0)

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
} pde_attrs_t;

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
} pte_attrs_t;
#define SET_PD(entry, attrs) do { \
    (entry)->present    = (attrs).present; \
    (entry)->rw         = (attrs).rw; \
    (entry)->user       = (attrs).user; \
    (entry)->write_thru = (attrs).write_thru; \
    (entry)->cache_dis  = (attrs).cache_dis; \
    (entry)->accessed   = (attrs).accessed; \
    (entry)->reserved   = 0; \
    (entry)->page_size  = (attrs).page_size; \
    (entry)->global     = (attrs).global; \
    (entry)->available  = (attrs).available; \
    (entry)->table_addr = (attrs).table_addr; \
} while(0)
extern pde_t pd[PAGE_DIRECTORY_SIZE];
extern pte_t first_pt[PAGE_TABLE_SIZE];

void paging_init(void);
void remove_id_map(void);
void _flush_tlb(void);

#endif