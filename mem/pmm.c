/*

Copyright 2024, 2025 Amar Djulovic <aaamargml@gmail.com>

This file is part of FloppaOS.

FloppaOS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

FloppaOS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with FloppaOS. If not, see <https://www.gnu.org/licenses/>.

*/

#include "pmm.h"
#include "../drivers/vga/vgahandler.h"
#include "../lib/logging.h"
#include <stdint.h>

struct buddy_allocator_t buddy;

// split a block into 2 blocks of half the size
void buddy_split(uintptr_t addr, uint32_t order) {
    // fetch addr of block to split (offset by half the current block size)
    uintptr_t buddy_addr = addr + (1 << (order - 1)) * PAGE_SIZE;
    
    // fetch page info structs for both blocks
    struct Page* page = phys_to_page_index(addr);
    struct Page* buddy_page = phys_to_page_index(buddy_addr);
    

    if (page && buddy_page) {
        page->order = order - 1;
        buddy_page->order = order - 1;
        
        // Add first block to free list of lower order
        page->next = buddy.free_list[order - 1];
        buddy.free_list[order - 1] = page;
    }
}

// merge a block with its buddy if possible
// recursively merges blocks until a block cannot be merged
void buddy_merge(uintptr_t addr, uint32_t order) {
    uintptr_t buddy_addr = addr ^ ((1 << order) * PAGE_SIZE);
    
    struct Page* page = phys_to_page_index(addr);
    struct Page* buddy_page = phys_to_page_index(buddy_addr);

    // check if buddy can be merged (must be free and same order)
    if (buddy_page && buddy_page->is_free && buddy_page->order == order) {
        // remove buddy from its free list
        struct Page** prev = &buddy.free_list[order];
        while (*prev && *prev != buddy_page) {
            prev = &(*prev)->next;
        }

        if (*prev) {
            *prev = buddy_page->next;
        }
        
        // fetch lower address of the two blocks
        uintptr_t merged_addr = (addr < buddy_addr) ? addr : buddy_addr;
        
        // recursively try to merge the larger block
        buddy_merge(merged_addr, order + 1);
    } else {
        // if can't merge, add to free list of current order
        page->next = buddy.free_list[order];
        buddy.free_list[order] = page;
    }
}
static void _add_page_to_free_list(struct Page* page, uintptr_t address, uint32_t order) {
    page->address = address;
    page->order = order;
    page->is_free = 1;
    page->next = buddy.free_list[order];
    buddy.free_list[order] = page;
}

static void _create_free_list(void) {
    for (uint32_t i = 0; i < buddy.total_pages; i++) {
        struct Page* page = &buddy.page_info[i];
        _add_page_to_free_list(page, buddy.memory_start + (i * PAGE_SIZE), MAX_ORDER);
    }
}

static void buddy_init(uint64_t total_memory, uintptr_t memory_start) {
    buddy.total_pages = total_memory / PAGE_SIZE;
    buddy.memory_start = memory_start;
    buddy.memory_end = buddy.memory_start + buddy.total_pages * PAGE_SIZE;

    buddy.page_info = (struct Page*)buddy.memory_start;
    buddy.memory_start += buddy.total_pages * sizeof(struct Page);

    _create_free_list();
}

static uint64_t iterate_mb_mmap(multiboot_info_t* mb_info) {
    uint8_t* mmap_ptr = (uint8_t*)mb_info->mmap_addr;
    uint8_t* mmap_end = mmap_ptr + mb_info->mmap_length;

    uint64_t total_memory = 0;

    while (mmap_ptr < mmap_end) {
        multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)mmap_ptr;
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
            total_memory += mmap->len;
        }
        mmap_ptr += mmap->size + sizeof(mmap->size);
    }

    return total_memory;
}

void pmm_init(multiboot_info_t* mb_info) {
    if (!mb_info || !(mb_info->flags & MULTIBOOT_INFO_MEM_MAP)) {
        log("pmm: Invalid Multiboot info!\n", RED);
        return;
    }

    uint64_t total_memory = iterate_mb_mmap(mb_info);
    buddy_init(total_memory, (uintptr_t)mb_info->mmap_addr);

    log_uint("pmm: Total pages: ", buddy.total_pages);
    log("pmm: memory available kb: ", LIGHT_GRAY);
    log_uint("", total_memory / 1024);
    log("pmm: memory available mb: ", LIGHT_GRAY);
    log_uint("", total_memory / 1024 / 1024);
    log("pmm: Buddy allocator initialized\n", GREEN);
}

void pmm_copy_page(void* dst, void* src) {
    for (int i = 0; i < PAGE_SIZE / sizeof(uint32_t); i++) {
        ((uint32_t*)dst)[i] = ((uint32_t*)src)[i];
    }
}

void* pmm_alloc_pages(uint32_t order, uint32_t count) {
    if (order > MAX_ORDER || count == 0) return NULL;

    void* first_page = NULL;

    for (uint32_t i = 0; i < count; i++) {
        void* page = NULL;
        for (uint32_t j = order; j <= MAX_ORDER; j++) {
            if (buddy.free_list[j]) {
                struct Page* p = buddy.free_list[j];
                buddy.free_list[j] = p->next;
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
int pmm_is_valid_addr(uintptr_t addr) {
    if (addr % PAGE_SIZE != 0) return 0;

    if (addr < buddy.memory_start || addr >= buddy.memory_end) return 0;

    uint32_t index = page_index(addr);
    if (index >= buddy.total_pages) return 0;

    return 1;
}

void* pmm_alloc_page(void) {
    return pmm_alloc_pages(0, 1);
}

void pmm_free_page(void* addr) {
    pmm_free_pages(addr, 0, 1);
}

int pmm_get_memory_size(void) {
    return buddy.total_pages * PAGE_SIZE;
}

unsigned int pmm_get_free_memory_size(void) {
    int free_pages = 0;
    for (int i = 0; i <= MAX_ORDER; i++) {
        struct Page* page = buddy.free_list[i];
        while (page) {
            free_pages++;
            page = page->next;
        }
    }
    return free_pages * PAGE_SIZE;
}

struct Page* pmm_get_last_used_page(void) {
    for (int page_index = buddy.total_pages - 1; page_index >= 0; page_index--) {
        struct Page* page = &buddy.page_info[page_index];
        if (!page->is_free) return page;
    }
    return NULL;
}

uintptr_t page_to_phys_addr(struct Page* page) {
    return page->address;
}

uint32_t page_index(uintptr_t addr) {
    return (addr - buddy.memory_start) / PAGE_SIZE;
}

struct Page* phys_to_page_index(uintptr_t addr) {
    uint32_t index = page_index(addr);
    if (index >= buddy.total_pages) return NULL;
    return &buddy.page_info[index];
}


void log_page_info(struct Page* page) {
    log_address("pmm: Page address: ", page->address);
    log_uint("pmm: Page order: ", page->order);
    log_uint("pmm: Page is_free: ", page->is_free);
    log_address("pmm: Page next: ", (uintptr_t)page->next);
}

void print_mem_info(void) {
    log("Memory Info:\n", LIGHT_GRAY);
    log("Total pages: ", LIGHT_GRAY);
    log_uint("", buddy.total_pages);
    log("\nFree pages: ", LIGHT_GRAY);
    for (int i = 0; i <= MAX_ORDER; i++) {
        log_uint("", (uint32_t)buddy.free_list[i]);
        log(" ", LIGHT_GRAY);
    }
    log("\n", LIGHT_GRAY);
}
