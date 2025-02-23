/* 

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

    pmm_init(...) sets up the buddy allocator using the multiboot info.
    
    pmm_alloc_page() allocates a single physical page.
    
    pmm_free_page(...) frees a single physical page.

------------------------------------------------------------------------------
*/

#include "pmm.h"
#include "../drivers/vga/vgahandler.h"
#include "../lib/logging.h"
#include "utils.h"

struct BuddyAllocator pmm_buddy;

void insert_free_page(uintptr_t addr, uint32_t order) {
    struct Page* new_page = (struct Page*)addr;
    new_page->address = addr;
    new_page->is_free = 1;
    new_page->order = order;
    new_page->next = pmm_buddy.free_list[order];
    pmm_buddy.free_list[order] = new_page;
}

void pmm_init(multiboot_info_t* mb_info) {
    if (!mb_info || !(mb_info->flags & MULTIBOOT_INFO_MEM_MAP)) {
        log_step("pmm: Invalid Multiboot info!\n", RED);
        return;
    }

    uintptr_t mmap_addr = (uintptr_t)mb_info->mmap_addr;
    uintptr_t mmap_end = mmap_addr + mb_info->mmap_length;
    multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)mmap_addr;

    flop_memset(&pmm_buddy, 0x00, sizeof(pmm_buddy));

    uint64_t total_memory = 0;
    while ((uintptr_t)mmap < mmap_end) {
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
            total_memory += mmap->len;
            pmm_buddy.total_pages += mmap->len / PAGE_SIZE;
        }
        mmap = (multiboot_memory_map_t*)((uintptr_t)mmap + mmap->size + sizeof(uint32_t));
    }

    pmm_buddy.memory_start = (uintptr_t)pmm_alloc_pages(0);
    pmm_buddy.memory_end = pmm_buddy.memory_start + total_memory;

    for (uint32_t i = 0; i <= MAX_ORDER; i++) {
        pmm_buddy.free_list[i] = NULL;
    }

    buddy_init(pmm_buddy.memory_start, pmm_buddy.total_pages);
    log_step("Physical Memory:", LIGHT_GRAY);
    log_uint("-> KB: ", total_memory / 1024);
    log_uint("-> MB: ", total_memory / (1024 * 1024));
    log_uint("-> GB: ", total_memory / (1024 * 1024 * 1024));
    log_uint("Total Usable Pages: ", pmm_buddy.total_pages); 
    log_step("pmm: Buddy allocator initialized\n", GREEN);
}

void buddy_init(uintptr_t start, uint32_t total_pages) {
    for (uint32_t i = 0; i <= MAX_ORDER; i++) {
        pmm_buddy.free_list[i] = NULL;
    }

    buddy_split(start, MAX_ORDER);
}

void* pmm_alloc_pages(uint32_t order) {
    if (order > MAX_ORDER) return NULL;

    for (uint32_t i = order; i <= MAX_ORDER; i++) {
        if (pmm_buddy.free_list[i]) {
            uintptr_t addr = (uintptr_t)pmm_buddy.free_list[i];
            pmm_buddy.free_list[i] = pmm_buddy.free_list[i]->next;

            while (i > order) {
                i--;
                buddy_split(addr, i);
            }
            struct Page* page = (struct Page*)addr;
            page->order = order;
            return (void*)addr;
        }
    }

    log_step("pmm: Out of memory!\n", RED);
    return NULL;
}

void* pmm_alloc_page(void) {
    return pmm_alloc_pages(0);
}

void pmm_free_pages(void* addr, uint32_t order) {
    if (!addr || order > MAX_ORDER) return;
    buddy_merge((uintptr_t)addr, order);
}

void pmm_free_page(void* addr) {
    pmm_free_pages(addr, 0);
}

void buddy_split(uintptr_t addr, uint32_t order) {
    uintptr_t buddy = addr + (1 << order) * PAGE_SIZE;
    struct Page* page = (struct Page*)addr;

    page->address = addr;
    page->is_free = 1;
    page->order = order;
    page->next = pmm_buddy.free_list[order];
    pmm_buddy.free_list[order] = page;

    page = (struct Page*)buddy;
    page->address = buddy;
    page->is_free = 1;
    page->order = order;
    page->next = pmm_buddy.free_list[order];
    pmm_buddy.free_list[order] = page;
}

void buddy_merge(uintptr_t addr, uint32_t order) {
    uintptr_t buddy = addr ^ (1 << order);
    struct Page* buddy_page = (struct Page*)buddy;

    if (buddy_page >= (struct Page*)pmm_buddy.memory_start && buddy_page < (struct Page*)pmm_buddy.memory_end) {
        struct Page* prev = NULL;
        struct Page* curr = pmm_buddy.free_list[order];

        while (curr) {
            if ((uintptr_t)curr == buddy) {
                if (prev) {
                    prev->next = curr->next;
                } else {
                    pmm_buddy.free_list[order] = curr->next;
                }

                if (addr < buddy) {
                    addr = addr;
                } else {
                    addr = buddy;
                }
                order++;

                buddy_merge(addr, order);
                return;
            }
            prev = curr;
            curr = curr->next;
        }
    }

    insert_free_page(addr, order);
}