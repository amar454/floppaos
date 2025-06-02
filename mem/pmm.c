/* 

Copyright 2024, 2025 Amar Djulovic <aaamargml@gmail.com>

This file is part of FloppaOS.

FloppaOS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

FloppaOS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with FloppaOS. If not, see <https://www.gnu.org/licenses/>.

------------------------------------------------------------------------------

pmm.c

    This is the physical memory manager for FloppaOS. 
    It uses the multiboot memory map to set up a buddy allocator, which helps manage physical pages.
    The virtual memory manager (vmm.c) uses this to allocate physical pages to virtual addresses.
    
    The buddy system splits memory into power-of-two sized blocks. When a block is freed, it's merged with its buddy if possible.

    - pmm_init(...) sets up the buddy allocator using the multiboot info.

    - pmm_alloc_pages(...) allocates a specified number of physical pages of the given order and count

    - pmm_free_pages(...) frees a specified number of physical pages of the given order.
    
    - pmm_alloc_page() allocates a single 4kb physical page.
    
    - pmm_free_page(...) frees a single 4kb physical page.

    - print_mem_info() prints memory usage information.

------------------------------------------------------------------------------
*/

#include "pmm.h"
#include "../drivers/vga/vgahandler.h"
#include "../lib/logging.h"
#include <stdint.h>
struct BuddyAllocator pmm_buddy;

// forward declarations
void pmm_init(multiboot_info_t* mb_info);
void* pmm_alloc_page(void);
void pmm_free_page(void* page);
void* pmm_alloc_pages(uint32_t order, uint32_t count);
void pmm_free_pages(void* addr, uint32_t order, uint32_t count);

uint32_t page_index(uintptr_t addr);

// Split a page into two pages of the same order
void buddy_split(uintptr_t addr, uint32_t order) {
    uintptr_t buddy_addr = addr + (1 << (order - 1)) * PAGE_SIZE;
    struct Page* page = phys_to_page_index(addr);
    struct Page* buddy_page = phys_to_page_index(buddy_addr);
    //  Update the order and link the pages
    if (page && buddy_page) {
        page->order = order - 1; 
        buddy_page->order = order - 1;
        page->next = pmm_buddy.free_list[order - 1]; 
        pmm_buddy.free_list[order - 1] = page;
    }
}

// Merge two pages of the same order into one page of the next higher order.
void buddy_merge(uintptr_t addr, uint32_t order) {
    uintptr_t buddy_addr = addr ^ (1 << order) * PAGE_SIZE;
    struct Page* page = phys_to_page_index(addr);
    struct Page* buddy_page = phys_to_page_index(buddy_addr);

    if (buddy_page && buddy_page->is_free && buddy_page->order == order) {
        // Remove buddy from free list
        struct Page** prev = &pmm_buddy.free_list[order];
        while (*prev && *prev != buddy_page) {
            prev = &(*prev)->next;
        }
        if (*prev) {
            *prev = buddy_page->next;
        }

        // Merge the pages
        uintptr_t merged_addr = (addr < buddy_addr) ? addr : buddy_addr;
        buddy_merge(merged_addr, order + 1);
    } else {
        page->next = pmm_buddy.free_list[order];
        pmm_buddy.free_list[order] = page;
    }
}

void pmm_init(multiboot_info_t* mb_info) {
    if (!mb_info || !(mb_info->flags & MULTIBOOT_INFO_MEM_MAP)) {
        log("pmm: Invalid Multiboot info!\n", RED);
        return;
    }

    uintptr_t mmap_addr = (uintptr_t)mb_info->mmap_addr;
    uintptr_t mmap_end = mmap_addr + mb_info->mmap_length;
    multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)mmap_addr;

    uint64_t total_memory = 0;


    while ((uintptr_t)mmap < mmap_end) {
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
            total_memory += mmap->len;
        }
        else if (mmap->type == MULTIBOOT_MEMORY_RESERVED) {
        }
        

        mmap = (multiboot_memory_map_t*)((uintptr_t)mmap + mmap->size + sizeof(mmap->size));
    }

    pmm_buddy.total_pages = total_memory / PAGE_SIZE;
    pmm_buddy.memory_start = (uintptr_t)mb_info->mmap_addr;
    pmm_buddy.memory_end = pmm_buddy.memory_start + pmm_buddy.total_pages * PAGE_SIZE;

    pmm_buddy.page_info = (struct Page*)pmm_buddy.memory_start;
    pmm_buddy.memory_start += pmm_buddy.total_pages * sizeof(struct Page);



    for (uint32_t i = 0; i < pmm_buddy.total_pages; i++) {
        struct Page* page = &pmm_buddy.page_info[i];
        page->address = pmm_buddy.memory_start + (i * PAGE_SIZE);
        page->order = MAX_ORDER;
        page->is_free = 1;
        page->next = pmm_buddy.free_list[MAX_ORDER];
        pmm_buddy.free_list[MAX_ORDER] = page;
    }

    log_uint("pmm: Total pages: ", pmm_buddy.total_pages);
    log("pmm: memory availabe kb: ", LIGHT_GRAY);
    log_uint("", total_memory / 1024);
    log("pmm: memory availabe mb: ", LIGHT_GRAY);
    log_uint("", total_memory / 1024 / 1024);
    log("pmm: Buddy allocator initialized\n", GREEN);
}

int pmm_get_memory_size(void) {
    return pmm_buddy.total_pages * PAGE_SIZE;
}

unsigned int pmm_get_free_memory_size(void) {
    int free_pages = 0;
    for (int i = 0; i <= MAX_ORDER; i++) {
        struct Page* page = pmm_buddy.free_list[i];
        while (page) {
            free_pages++;
            page = page->next;
        }
    }
    return free_pages * PAGE_SIZE;
}

struct Page* pmm_get_last_used_page(void) {
    for (int page_index = pmm_buddy.total_pages - 1; page_index >= 0; page_index--) {
        struct Page* page = &pmm_buddy.page_info[page_index];
        if (!page->is_free) {
            return page;
        }
    }
    return NULL;
}

inline uintptr_t page_to_phys_addr(struct Page* page) {
    return page->address;
}

uint32_t page_index(uintptr_t addr) {
    return (addr - pmm_buddy.memory_start) / PAGE_SIZE;
}

struct Page* phys_to_page_index(uintptr_t addr) {
    uint32_t index = page_index(addr);
    if (index >= pmm_buddy.total_pages) return NULL;
    return &pmm_buddy.page_info[index];
}

void log_page_info(struct Page* page) {
    log_address("pmm: Page address: ", page->address);
    log_uint("pmm: Page order: ", page->order);
    log_uint("pmm: Page is_free: ", page->is_free);
    log_address("pmm: Page next: ", (uintptr_t)page->next);
}

void* pmm_alloc_pages(uint32_t order, uint32_t count) {
    if (order > MAX_ORDER || count == 0) return NULL;

    void* first_page = NULL;
    void* current_page = NULL;

    for (uint32_t i = 0; i < count; i++) {
        void* page = NULL;
        for (uint32_t j = order; j <= MAX_ORDER; j++) {
            if (pmm_buddy.free_list[j]) {
                struct Page* p = pmm_buddy.free_list[j];
                pmm_buddy.free_list[j] = p->next;
                p->is_free = 0;
                p->order = order;

                while (j > order) {
                    j--;
                    buddy_split(p->address, j);
                }

                page = (void*)p->address;
                break;
            }
        }

        if (!page) {
            if (first_page) {
                pmm_free_pages(first_page, order, i);
            }
            log("pmm: Out of memory!\n", RED);
            return NULL;
        }

        if (!first_page) {
            first_page = page;
        }
    }

    return first_page;
}

void pmm_free_pages(void* addr, uint32_t order, uint32_t count) {
    if (!addr || order > MAX_ORDER || count == 0) return;

    uintptr_t current_addr = (uintptr_t)addr;
    for (uint32_t i = 0; i < count; i++) {
        struct Page* page = phys_to_page_index(current_addr);
        if (!page) return;

        page->is_free = 1;
        buddy_merge(page->address, order);
        current_addr += (1 << order) * PAGE_SIZE;
    }
}


// allocate a single 4kb page 
void* pmm_alloc_page(void) {
    return pmm_alloc_pages(0, 1);
}

// free a single 4kb at *addr
void pmm_free_page(void* addr) {
    pmm_free_pages(addr, 0, 1);
}


void print_mem_info() {
    log("Memory Info:\n", LIGHT_GRAY);
    log("Total pages: ", LIGHT_GRAY);
    log_uint("", pmm_buddy.total_pages);
    log("\nFree pages: ", LIGHT_GRAY);
    for (int i = 0; i <= MAX_ORDER; i++) {
        log_uint("", (uint32_t)pmm_buddy.free_list[i]);
        log(" ", LIGHT_GRAY);
    }
    log("\n", LIGHT_GRAY);
}


