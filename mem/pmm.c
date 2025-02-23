/* 

Copyright 2024-25 Amar Djulovic <aaamargml@gmail.com>

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

    pmm_alloc_pages(...) allocates a block of physical memory of the specified order.

    pmm_free_pages(...) frees a block of physical memory of the specified order.
    
    pmm_alloc_page() allocates a single physical page.
    
    pmm_free_page(...) frees a single physical page.

    print_mem_info() prints memory usage information.

------------------------------------------------------------------------------
*/

#include "pmm.h"
#include "../drivers/vga/vgahandler.h"
#include "../lib/logging.h"
#include "utils.h"
#include <stdint.h>
#include "../apps/echo.h"
struct BuddyAllocator pmm_buddy;

// forward declarations
void pmm_init(multiboot_info_t* mb_info);
void* pmm_alloc_page();
void pmm_free_page(void* page);
void* pmm_alloc_pages(uint32_t order);
void pmm_free_pages(void* addr, uint32_t order);
uint32_t page_index(uintptr_t addr);

/**
 * @name pmm_init
 * @author Amar Djulovic <aaamargml@gmail.com>
 *
 * @brief Initialize the physical memory manager
 *  
 * @param mb_info Multiboot information structure
 * @return void
 *
 * @note This function sets up the buddy allocator and initializes the physical memory manager.
 * @note It uses the multiboot memory map to determine the available memory.
 * @note It uses the buddy allocator to manage physical pages.
 * @note The buddy allocator is a binary tree-based allocator that efficiently allocates and deallocates memory blocks of various sizes.
 * @note It reduces memory fragmentation by merging free blocks when possible.
 */
void pmm_init(multiboot_info_t* mb_info) {
    if (!mb_info || !(mb_info->flags & MULTIBOOT_INFO_MEM_MAP)) {
        log_step("pmm: Invalid Multiboot info!\n", RED);
        return;
    }

    // Iterate over the memory map and count total available memory
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

    // Initialize buddy allocator metadata
    pmm_buddy.total_pages = total_memory / PAGE_SIZE;
    pmm_buddy.memory_start = (uintptr_t)mb_info->mmap_addr;
    pmm_buddy.memory_end = pmm_buddy.memory_start + pmm_buddy.total_pages * PAGE_SIZE;

    // Allocate and initialize page metadata array
    pmm_buddy.page_info = (struct Page*)pmm_buddy.memory_start;
    pmm_buddy.memory_start += pmm_buddy.total_pages * sizeof(struct Page);

    // Initialize free list
    for (uint32_t i = 0; i < pmm_buddy.total_pages; i++) {
        struct Page* page = &pmm_buddy.page_info[i];
        page->address = pmm_buddy.memory_start + (i * PAGE_SIZE);
        page->order = MAX_ORDER;
        page->is_free = 1;
        page->next = pmm_buddy.free_list[MAX_ORDER];
        pmm_buddy.free_list[MAX_ORDER] = page;
    }

    // Log memory statistics
    log_address("pmm: Memory start addr: ", pmm_buddy.memory_start);
    log_address("pmm: Memory end addr: ", pmm_buddy.memory_end);
    log_uint("pmm: Total memory (KB): ", total_memory / 1024);
    log_uint("pmm: Total memory (MB): ", total_memory / (1024 * 1024));
    log_uint("pmm: Total memory (GB): ", total_memory / (1024 * 1024 * 1024));
    log_uint("pmm: Total pages: ", pmm_buddy.total_pages);

    log_step("pmm: Buddy allocator initialized\n", GREEN);
}

/* Return the index of the page that contains the given address */
uint32_t page_index(uintptr_t addr) {
    return (addr - pmm_buddy.memory_start) / PAGE_SIZE;
}
/* Returns the page struct for a given address */
struct Page* page_from_address(uintptr_t addr) {
    uint32_t index = page_index(addr);
    if (index >= pmm_buddy.total_pages) return NULL;
    return &pmm_buddy.page_info[index];
}

/**
 * @name pmm_alloc_pages
 * @author Amar Djulovic <aaamargml@gmail.com>
 *
 * @brief Allocate physical pages based off of an order
 
 * @param order 
 * @return void* 
 * @returns a pointer to the allocated page
 *
 * @note the orders are as follows:
 * @note 0 - 4KB
 * @note 1 - 8KB
 * @note 2 - 16KB
 * @note 3 - 32KB
 * @note 4 - 64KB
 * @note 5 - 128KB
 * @note 6 - 256KB
 * @note 7 - 512KB
 * @note 8 - 1024KB
 * @note 9 - 2048KB
 * @note 10 - 4096KB
 * @note these are called "orders" because they are powers of two
 */
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

/**
 * @name pmm_alloc_pages
 * @author Amar Djulovic <aaamargml@gmail.com> 
 *
 * @brief Free pages based off of an order
 * 
 * @param addr 
 * @param order 
 * @return void
 *
 * @note the orders are the same as above.
 */
void pmm_free_pages(void* addr, uint32_t order) {
    if (!addr || order > MAX_ORDER) return;

    struct Page* page = page_from_address((uintptr_t)addr);
    if (!page) return;

    page->is_free = 1;
    buddy_merge(page->address, order);
}


/**
 * @name buddy_split
 * @author Amar Djulovic <aaamargml@gmail.com>
 *
 * @brief Split a page into two pages of the same order
 * 
 * @param addr 
 * @param order 
 *
 * @note the orders are the same as above.
 */
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
/**
 * @name buddy_merge
 * @author Amar Djulovic <aaamargml@gmail.com>
 *
 * @brief Merge two pages of the same order into one page of the next higher order
 * 
 * @param addr 
 * @param order 
 *
 * @note the orders are the same as above.
 * @note this function is called recursively until the order is 0
 */
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

// helper functions for allocating singular o(0) pages
void* pmm_alloc_page() {
    return pmm_alloc_pages(0);
}
void pmm_free_page(void* addr) {
    pmm_free_pages(addr, 0);
}

// print memory usage info
void print_mem_info() {
    echo_f("Total memory: %d KB\n", pmm_buddy.total_pages * PAGE_SIZE / 1024);
    echo_f("Free memory: %d KB\n", (pmm_buddy.total_pages - sizeof(pmm_buddy.free_list)) * PAGE_SIZE / 1024);
    echo_f("Used memory: %d KB\n", sizeof(pmm_buddy.free_list) * PAGE_SIZE / 1024);

}