#ifndef PMM_H
#define PMM_H
#include <stdint.h>
#include "../multiboot/multiboot.h"
#define TOTAL_PAGES 1024
#define BITMAP_SIZE (TOTAL_PAGES / 32)
// Physical memory bitmap to keep track of allocated pages
//static uint32_t memory_bitmap[TOTAL_PAGES / 32];

void pmm_init(multiboot_info_t* mb_info);
void* pmm_alloc_page();
void pmm_free_page(void* ptr);

#endif