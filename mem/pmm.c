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
#include <stdint.h>
#include "../apps/echo.h"
struct BuddyAllocator pmm_buddy;
void pmm_init(multiboot_info_t* mb_info);
void* pmm_alloc_page();
void pmm_free_page(void* page);
void* pmm_alloc_pages(uint32_t order);
void pmm_free_pages(void* addr, uint32_t order);
uint32_t page_index(uintptr_t addr);
void pmm_init(multiboot_info_t* mb_info) {
    if (!mb_info || !(mb_info->flags & MULTIBOOT_INFO_MEM_MAP)) {
        log_step("pmm: Invalid Multiboot info!\n", RED);
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
        mmap = (multiboot_memory_map_t*)((uintptr_t)mmap + mmap->size + sizeof(uint32_t));
    }
    pmm_buddy.total_pages = total_memory / PAGE_SIZE;
    pmm_buddy.memory_start = (uintptr_t)mb_info->mmap_addr;
    pmm_buddy.memory_end = pmm_buddy.memory_start + pmm_buddy.total_pages * PAGE_SIZE;
    // Allocate page metadata array
    pmm_buddy.page_info = (struct Page*)pmm_buddy.memory_start;
    pmm_buddy.memory_start += pmm_buddy.total_pages * sizeof(struct Page);
    for (uint32_t i = 0; i < pmm_buddy.total_pages; i++) {
        pmm_buddy.page_info[i].address = pmm_buddy.memory_start + (i * PAGE_SIZE);
        pmm_buddy.page_info[i].order = 0;
        pmm_buddy.page_info[i].is_free = 1;
        pmm_buddy.page_info[i].next = NULL;
    }
    for (uint32_t i = 0; i < pmm_buddy.total_pages; i++) {
        struct Page* page = &pmm_buddy.page_info[i];
        page->order = MAX_ORDER; 
        page->is_free = 1;
        page->next = pmm_buddy.free_list[MAX_ORDER];
        pmm_buddy.free_list[MAX_ORDER] = page;
    }
    log_address("pmm: Memory end addr: ", pmm_buddy.memory_end);
    log_address("pmm: Memory start addr: ", pmm_buddy.memory_start);
    log_uint("pmm: Total memory (KB): ", total_memory / 1024);
    log_uint("pmm: Total memory (MB): ", total_memory / (1024 * 1024));
    log_uint("pmm: Total memory (GB): ", total_memory / (1024 * 1024 * 1024));
    log_uint("pmm: Total pages", pmm_buddy.total_pages);

    log_step("pmm: Buddy allocator initialized\n", GREEN);
}

uint32_t page_index(uintptr_t addr) {
    return (addr - pmm_buddy.memory_start) / PAGE_SIZE;
}

struct Page* page_from_address(uintptr_t addr) {
    uint32_t index = page_index(addr);
    if (index >= pmm_buddy.total_pages) return NULL;
    return &pmm_buddy.page_info[index];
}

void* pmm_alloc_pages(uint32_t order) {
    if (order > MAX_ORDER) return NULL;

    for (uint32_t i = order; i <= MAX_ORDER; i++) {
        if (pmm_buddy.free_list[i]) {
            struct Page* page = pmm_buddy.free_list[i];
            pmm_buddy.free_list[i] = page->next;
 
            page->is_free = 0;
            page->order = order;

            while (i > order) {
                i--;
                buddy_split(page->address, i);
            }

            return (void*)page->address;
        }
    }

    log_step("pmm: Out of memory!\n", RED);
    return NULL;
}

void pmm_free_pages(void* addr, uint32_t order) {
    if (!addr || order > MAX_ORDER) return;

    struct Page* page = page_from_address((uintptr_t)addr);
    if (!page) return;

    page->is_free = 1;
    buddy_merge(page->address, order);
}

void buddy_split(uintptr_t addr, uint32_t order) {
    uintptr_t buddy_addr = addr + (1 << (order - 1)) * PAGE_SIZE;
    struct Page* page = page_from_address(addr);
    struct Page* buddy_page = page_from_address(buddy_addr);

    if (page && buddy_page) {
        page->order = order - 1;
        buddy_page->order = order - 1;
        page->next = pmm_buddy.free_list[order - 1];
        pmm_buddy.free_list[order - 1] = page;
    }
}

void buddy_merge(uintptr_t addr, uint32_t order) {
    uintptr_t buddy_addr = addr ^ (1 << order) * PAGE_SIZE;
    struct Page* page = page_from_address(addr);
    struct Page* buddy_page = page_from_address(buddy_addr);

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
void* pmm_alloc_page() {
    return pmm_alloc_pages(0);
}
void pmm_free_page(void* addr) {
    pmm_free_pages(addr, 0);
}
void print_mem_info() {
    echo_f("Total memory: %d KB\n", pmm_buddy.total_pages * PAGE_SIZE / 1024);
    echo_f("Free memory: %d KB\n", (pmm_buddy.total_pages - sizeof(pmm_buddy.free_list)) * PAGE_SIZE / 1024);
    echo_f("Used memory: %d KB\n", sizeof(pmm_buddy.free_list) * PAGE_SIZE / 1024);

}