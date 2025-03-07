/* 

Copyright 2024-25 Amar Djulovic <aaamargml@gmail.com>

This file is part of FloppaOS.

FloppaOS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

FloppaOS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with FloppaOS. If not, see <https://www.gnu.org/licenses/>.

------------------------------------------------------------------------------

vmm.c

    This is the virtual memory manager for floppaOS.
    It takes physical pages and makes virtual address spaces for each process.
    In an operating system, this allows each process to have it's own isolated memory space to prevent memory leaks/overwrites.
    
    set_page(...) sets the flags for each page table entry using the PageAttributes struct.
    
    vmm_map_page(...) maps a virtual address to a physical page.

    vmm_init() initializes the virtual memory manager.

    vmm_malloc(...) allocates the amount of memory given in the size parameter.
        - It first determines the amount of physical pages needed.
        - Then, it maps those physical pages to a virtual address.
        - Lastly, it returns a pointer to the start of the virtual address... (void *)start_virt;
        - Returns NULL if there isn't enough pages for the size given.

------------------------------------------------------------------------------
*/

#include "vmm.h"
#include "../drivers/vga/vgahandler.h"
#include "../apps/echo.h"
#include "paging.h"
#include "pmm.h"
#include "utils.h"
#include <stdint.h>
#include <stddef.h>
#include "../lib/logging.h"
#include "../lib/str.h"
// Set flags for a page table entry
static void set_page(PTE *pte, PageAttributes attrs) {
    if (!pte) return;
    *(PageAttributes *)pte = attrs;
    //echo("Page flags set.\n", CYAN);
}

/**
 * @name vmm_map_page
 * @author Amar Djulovic <aaamargml@gmail.com>
 *
 * @brief Maps a virtual address to a physical page.
 * 
 * @param page_directory - The page directory to map the page to.
 * @param virt_addr - The virtual address to map the page to.
 * @param phys_addr - The physical address to map the page to.
 * @param attrs - The attributes to set for the page.
 */
void vmm_map_page(PDE *page_directory, uintptr_t virt_addr, uintptr_t phys_addr, PageAttributes attrs) { // TODO: add kernel and user space seperation
    if (!page_directory) {
        log_step("Invalid page directory!\n", RED);
        return;
    }

    uint32_t dir_idx = virt_addr >> 22;
    uint32_t table_idx = (virt_addr >> 12) & 0x3FF;

    if (!page_directory[dir_idx].present) {
        // Allocate a new page table if not present
        PTE *new_table = (PTE *)pmm_alloc_page();
        if (!new_table) {
            log_step("Failed to allocate page table!\n", RED);
            return;
        }
        flop_memset(new_table, 0, PAGE_SIZE); // Clear the new table

        // Set the new page table in the page directory
        page_directory[dir_idx].present = 1;
        page_directory[dir_idx].rw = 1;
        page_directory[dir_idx].user = 1;
        page_directory[dir_idx].table_addr = (uintptr_t)new_table >> 12;
    }

    // Map the physical page in the page table
    PTE *page_table = (PTE *)(page_directory[dir_idx].table_addr << 12);
    set_page(&page_table[table_idx], (PageAttributes){
        .present = attrs.present,
        .rw = attrs.rw,
        .user = attrs.user,
        .frame_addr = phys_addr >> 12
    });
}

/**
 * @name vmm_init
 * @author Amar Djulovic <aaamargml@gmail.com>
 *
 * @brief Initializes the virtual memory manager.
 *
 * @note This function doesn't really do anything besides initializing the page directory.
 */
void vmm_init() {
    log_step("Initializing vmm...\n ", LIGHT_GRAY);
    flop_memset(page_directory, 0, sizeof(PDE) * PAGE_DIRECTORY_SIZE);  // Initialize the page directory
    log_step("VMM initialized.\n", GREEN);
}

// Allocate virtual memory by mapping physical pages
void *vmm_malloc(uint32_t size) {
    log_step("Allocating virtual memory...\n", WHITE);
    
    uint32_t pages_needed = (size + PAGE_SIZE - 1) / PAGE_SIZE;  // Calculate required pages
    uintptr_t start_virt = 0;  // Start of virtual memory allocation
    uint32_t found_pages = 0;

    // Iterate through page directory and find free contiguous pages
    for (uint32_t i = 0; i < PAGE_DIRECTORY_SIZE; i++) {
        for (uint32_t j = 0; j < PAGE_TABLE_SIZE; j++) {
            if (!page_tables[i][j].present) {  // Free page found
                if (found_pages == 0) {
                    start_virt = (i << 22) | (j << 12);  // Start tracking contiguous free space
                }

                found_pages++;

                // If enough contiguous pages found, allocate them
                if (found_pages == pages_needed) {
                    log_step("Found enough contiguous virtual pages!\n", GREEN);

                    // Allocate physical memory and map pages
                    for (uint32_t k = 0; k < pages_needed; k++) {
                        uintptr_t virt_addr = start_virt + (k * PAGE_SIZE);
                        void *phys_addr = pmm_alloc_page();  // Allocate a physical page

                        // Map the virtual address to the physical page
                        vmm_map_page(page_directory, virt_addr, (uintptr_t)phys_addr, (PageAttributes){
                            .present = 1,
                            .rw = 1,
                            .user = 1
                        });
                    }

                    log_step("Virtual memory allocated successfully.\n", GREEN);
                    return (void *)start_virt;
                }
            } else {
                found_pages = 0;  // Reset if not contiguous
            }
        }
    }

    log_step("Virtual memory allocation failed. Not enough contiguous space.\n", RED);
    return NULL;
}


// Free virtual memory and unmap physical pages
void vmm_free(void *start_virt, uint32_t size) {
    uint32_t pages_to_free = (size + PAGE_SIZE - 1) / PAGE_SIZE;

    for (uint32_t i = 0; i < pages_to_free; i++) {
        uintptr_t virt_addr = (uintptr_t)start_virt + i * PAGE_SIZE;
        uint32_t dir_idx = virt_addr >> 22;
        uint32_t table_idx = (virt_addr >> 12) & 0x3FF;

        // Check if the page is mapped
        if (page_tables[dir_idx][table_idx].present) {
            void *phys_addr = (void *)(page_tables[dir_idx][table_idx].frame_addr << 12);
            pmm_free_page(phys_addr);  // Free the physical page
            flop_memset(&page_tables[dir_idx][table_idx], 0, sizeof(PTE));  // Clear the page table entry
        }
    }

    echo("Virtual memory freed.\n", GREEN);
}

// Resolve physical address for a given virtual address
uintptr_t vmm_get_physical_address(PDE *page_directory, uintptr_t virt_addr) {
    uint32_t dir_idx = virt_addr >> 22;
    uint32_t table_idx = (virt_addr >> 12) & 0x3FF;

    if (!page_directory || !page_directory[dir_idx].present) {
        return 0;
    }

    PTE *page_table = (PTE *)(uintptr_t)(page_directory[dir_idx].table_addr << 12);
    if (!page_table || !page_table[table_idx].present) {
        return 0;
    }

    return (page_table[table_idx].frame_addr << 12) | (virt_addr & 0xFFF);
}
