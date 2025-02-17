/* 

This file is part of FloppaOS.

FloppaOS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

FloppaOS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with FloppaOS. If not, see <https://www.gnu.org/licenses/>.

------------------------------------------------------------------------------

pmm.c

    This is the physical memory manager for FloppaOS. 
    It takes the multiboot memory map, initializes a buddy allocator, gives data about memory size, and allows physical pages to be allocated.
    The virtual memory manager (vmm.c in this same directory) allocates physical pages to virtual address spaces.
    
    The buddy system is a memory allocation algorithm that divides memory into partitions to try to satisfy a memory request as suitably as possible. 
    Each partition is a power of two in size, and each allocation is rounded up to the nearest power of two. 
    When a partition is freed, it is merged with its buddy (if the buddy is also free) to form a larger partition.

    pmm_init(...) takes in the multiboot info pointer and checks if it's valid. The steps it takes are:
        - checks if the pointer is null
        - displays info flags
        - checks if multiboot provides a memory map (it should)
        - prints info about memory map, memory regions, and total memory size in kb, mb, and gb
    
    pmm_alloc_page() simply allocates a physical page and returns a pointer to it.

    pmm_free_page(...) deallocates a physical page.

    pmm_is_page_free(...) is a debugging utility to check if a page is free 

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
    log_uint("Total Usable Pages: ", pmm_buddy.total_pages); // 
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
            return (void*)addr;
        }
    }

    //log_step("pmm: Out of memory!\n", RED);
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
    page->next = pmm_buddy.free_list[order];
    pmm_buddy.free_list[order] = page;

    page = (struct Page*)buddy;
    page->address = buddy;
    page->is_free = 1;
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