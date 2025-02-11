#include <stdint.h>
#include <stddef.h>
#include "pmm.h"
#include "paging.h"
#include "../multiboot/multiboot.h"
#include "../drivers/vga/vgahandler.h"
#include "../apps/echo.h"
#include "utils.h"
#include "../lib/logging.h"

uint32_t memory_bitmap[BITMAP_SIZE]; 
uint32_t total_pages = 0;  // Total available pages

void pmm_init(multiboot_info_t* mb_info) {
    if (mb_info == NULL) {
        log_step("Multiboot info pointer is NULL", RED);
        return;
    }

    log_uint("Multiboot Info Flags: ", mb_info->flags);

    if (!(mb_info->flags & MULTIBOOT_INFO_MEM_MAP)) {
        log_step("No memory map provided by Multiboot!\n", RED);
        return;
    }

    log_address("Memory Map Address: ", mb_info->mmap_addr);
    log_uint("Memory Map Length: ", mb_info->mmap_length);  

    // Initialize all pages as used (1)
    flop_memset(memory_bitmap, 0x00, sizeof(memory_bitmap)); 

    uintptr_t mmap_addr = (uintptr_t)mb_info->mmap_addr;
    uintptr_t mmap_end = mmap_addr + mb_info->mmap_length;
    multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)mmap_addr;

    uint64_t total_memory = 0;

    while ((uintptr_t)mmap < mmap_end) {
        log_step("Memory Region:", LIGHT_GRAY);
        log_uint("Start Address: ", (uint32_t)mmap->addr);
        log_uint("Length: ", (uint32_t)mmap->len);
        log_uint("Type: ", (uint32_t)mmap->type);

        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
            uint64_t addr = mmap->addr;
            uint64_t len = mmap->len;
            total_memory += len;

            while (len >= PAGE_SIZE) {
                uint32_t page = addr / PAGE_SIZE;

                if (page >= TOTAL_PAGES) {
                    log_step("pmm: Page index out of bounds, skipping.", YELLOW);
                    break;
                }

                memory_bitmap[page / 32] &= ~(1 << (page % 32)); // Mark as free
                total_pages++;
                addr += PAGE_SIZE;
                len -= PAGE_SIZE;
            }
        }

        mmap = (multiboot_memory_map_t*)((uintptr_t)mmap + mmap->size + sizeof(uint32_t)); 
    }

    log_step("Physical Memory:", LIGHT_GRAY);
    log_uint("-> KB: ", total_memory / 1024);
    log_uint("-> MB: ", total_memory / (1024 * 1024));
    log_uint("-> GB: ", total_memory / (1024 * 1024 * 1024));
    log_uint("Total Usable Pages: ", total_pages);

    log_step("pmm: Physical memory manager initialized.\n", GREEN);
}

void* pmm_alloc_page() {
    log_step("pmm: Attempting to allocate page...", LIGHT_GRAY);
    
    for (uint32_t i = 0; i < total_pages; i++) {
        uint32_t bit_index = i % 32;
        uint32_t bitmap_index = i / 32;

        if (!(memory_bitmap[bitmap_index] & (1 << bit_index))) {
            memory_bitmap[bitmap_index] |= (1 << bit_index); // Mark as used

            log_address("pmm: Allocated Page at: ", (uintptr_t)(i * PAGE_SIZE));
            return (void*)(uintptr_t)(i * PAGE_SIZE);
        }
    }

    log_step("pmm: Out of physical memory.\n", RED);
    return NULL;
}

// Free a single physical page
void pmm_free_page(void* ptr) {
    if (!ptr) return;

    uint32_t page = (uintptr_t)ptr / PAGE_SIZE;
    if (page >= total_pages) {
        log_step("pmm: Invalid page free request!", RED);
        return;
    }

    uint32_t bit_index = page % 32;
    uint32_t bitmap_index = page / 32;

    memory_bitmap[bitmap_index] &= ~(1 << bit_index); // Mark as free
    log_address("pmm: Freed Page at: ", (uintptr_t)ptr);
}

// Check if a page is free (debugging utility)
int pmm_is_page_free(uintptr_t page) {
    if (page >= total_pages) return 0; // Out of bounds

    uint32_t bit_index = page % 32;
    uint32_t bitmap_index = page / 32;

    return !(memory_bitmap[bitmap_index] & (1 << bit_index)); // 1 if free, 0 if used
}
