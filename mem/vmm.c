/* 

Copyright 2024, 2025 Amar Djulovic <aaamargml@gmail.com>

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

 * @brief Maps a virtual address to a physical page.
 * 
 * @param page_directory - The page directory to map the page to.
 * @param virt_addr - The virtual address to map the page to.
 * @param phys_addr - The physical address to map the page to.
 * @param attrs - The attributes to set for the page.
 */
 void vmm_map_page(PDE *page_directory, uintptr_t virt_addr, uintptr_t phys_addr, PageAttributes attrs) {
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
    PTE *page_table = (PTE *)((uintptr_t)page_directory[dir_idx].table_addr << 12);

    if (page_table[table_idx].present) {
        // Avoid double mapping
        if ((page_table[table_idx].frame_addr << 12) != phys_addr) {
            log_step("Page already mapped, skipping remap!\n", YELLOW);
            return;
        }
    }

    set_page(&page_table[table_idx], (PageAttributes){
        .present = attrs.present,
        .rw = attrs.rw,
        .user = attrs.user,
        .frame_addr = phys_addr >> 12
    });
}

void vmm_unmap_page(PDE *page_directory, uintptr_t virt_addr) {
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

    // Unmap the physical page in the page table
    PTE *page_table = (PTE *)((uintptr_t)(page_directory[dir_idx].table_addr << 12));
    if (page_table[table_idx].present) {
        page_table[table_idx].present = 0;
    }
}

void vmm_init() {
    log_step("Initializing vmm...\n ", LIGHT_GRAY);
    flop_memset(page_directory, 0, sizeof(PDE) * PAGE_DIRECTORY_SIZE);  // Initialize the page directory
    log_step("VMM initialized.\n", GREEN);
}

// Allocate virtual memory by mapping physical pages
void *vmm_malloc(uint32_t size) {
    log_step("Allocating virtual memory...\n", WHITE);
    
    if (size == 0) return NULL;

    // align to page size
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    uint32_t pages_needed = size / PAGE_SIZE;
    uintptr_t start_virt = 0;
    uint32_t found_pages = 0;
    // Iterate through page tables to find contiguous pages
    for (uint32_t i = 0; i < PAGE_DIRECTORY_SIZE; i++) {
        // Check if the page table is present
        for (uint32_t j = 0; j < PAGE_TABLE_SIZE; j++) {
            // Check if the page is present
            if (!page_tables[i][j].present) {
                // Allocate a new page table if not present
                if (found_pages == 0) {
                    start_virt = (i << 22) | (j << 12);
                }

                found_pages++;

                if (found_pages == pages_needed) {
                    // Allocate and map physical pages
                    for (uint32_t k = 0; k < pages_needed; k++) {
                        uintptr_t virt_addr = start_virt + (k * PAGE_SIZE);
                        void *phys_addr = pmm_alloc_page();
                        if (!phys_addr) {
                            log_step("Failed to allocate physical memory!\n", RED);
                            return NULL;
                        }

                        // Map virtual address to physical address
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
                found_pages = 0;
            }
        }
    }

    log_step("Virtual memory allocation failed.\n", RED);
    return NULL;
}

// Free virtual memory and unmap physical pages
void vmm_free(void *start_virt, uint32_t size) {
    if (!start_virt || size == 0) return;

    uint32_t pages_to_free = (size + PAGE_SIZE - 1) / PAGE_SIZE;

    for (uint32_t i = 0; i < pages_to_free; i++) {
        uintptr_t virt_addr = (uintptr_t)start_virt + (i * PAGE_SIZE);
        uint32_t dir_idx = virt_addr >> 22;
        uint32_t table_idx = (virt_addr >> 12) & 0x3FF;

        if (page_tables[dir_idx][table_idx].present) {
            void *phys_addr = (void *)(uintptr_t)(page_tables[dir_idx][table_idx].frame_addr << 12);
            pmm_free_page(phys_addr);
            flop_memset(&page_tables[dir_idx][table_idx], 0, sizeof(PTE));
        }
    }

    log_step("Virtual memory freed.\n", GREEN);
}

// Get physical address of a virtual address
 uintptr_t vmm_virt_to_phys(PDE *page_directory, uintptr_t virt_addr) {
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

// map a region of memory in page directory for a struct of size struct_size
void* vmm_map_struct_region(PDE *page_directory, size_t struct_size) {
    if (!page_directory || struct_size == 0) {
        log_step("Invalid parameters for struct region mapping!\n", RED);
        return NULL;
    }

    // Calculate number of pages needed for the struct
    uint32_t pages_needed = (struct_size + PAGE_SIZE - 1) / PAGE_SIZE;
    
    // Allocate virtual memory for the struct
    void *struct_region = vmm_malloc(pages_needed * PAGE_SIZE);
    if (!struct_region) {
        log_step("Failed to allocate struct region!\n", RED);
        return NULL;
    }

    // Initialize the memory region to zero
    flop_memset(struct_region, 0, pages_needed * PAGE_SIZE);
    
    log_step("Struct region mapped successfully.\n", GREEN);
    return struct_region;
}

void vmm_unmap_struct_region(PDE *page_directory, void *struct_region, size_t struct_size) {
    if (!page_directory || !struct_region) {
        log_step("Invalid parameters for struct region unmapping!\n", RED);
        return;
    }

    // Calculate number of pages needed for the struct
    uint32_t pages_needed = (struct_size + PAGE_SIZE - 1) / PAGE_SIZE;
    
    // Free the virtual memory for the struct
    vmm_free(struct_region, pages_needed * PAGE_SIZE);

    log_step("Struct region unmapped successfully.\n", GREEN);
}

void *vmm_reserve_region(PDE *page_directory, size_t size) {
    if (!page_directory || size == 0) {
        log_step("Invalid parameters for reserving region!\n", RED);
        return NULL;
    }

    // Align size to page size
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    uint32_t pages_needed = size / PAGE_SIZE;
    
    uintptr_t start_virt = 0;
    uint32_t found_pages = 0;

    // Iterate through page tables to find contiguous pages
    for (uint32_t i = 0; i < PAGE_DIRECTORY_SIZE; i++) {
        for (uint32_t j = 0; j < PAGE_TABLE_SIZE; j++) {
            if (!page_tables[i][j].present) { 
                if (found_pages == 0) {
                    start_virt = (i << 22) | (j << 12);
                }
                found_pages++;
                
                if (found_pages == pages_needed) {
                    log_step("Virtual region reserved successfully.\n", GREEN);
                    return (void *)start_virt;
                }
            } else {
                found_pages = 0;
            }
        }
    }

    log_step("Failed to reserve virtual region.\n", RED);
    return NULL;
}


void vmm_free_region(PDE *page_directory, void *virt_addr, size_t size) {
    if (!page_directory || !virt_addr || size == 0) {
        log_step("Invalid parameters for freeing region!\n", RED);
        return;
    }

    // Align size to page size
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    uint32_t pages_to_free = size / PAGE_SIZE;

    // Calculate the start of the page range to free
    uintptr_t start_virt = (uintptr_t)virt_addr;
    uint32_t start_index = (start_virt >> 22) & 0x3FF;  
    uint32_t start_table = (start_virt >> 12) & 0x3FF;  

    for (uint32_t i = start_index; i < PAGE_DIRECTORY_SIZE; i++) {
        for (uint32_t j = (i == start_index ? start_table : 0); j < PAGE_TABLE_SIZE; j++) {
            // Calculate the physical address
            uintptr_t page_addr = (i << 22) | (j << 12);
            if (page_tables[i][j].present) {
                // Mark the page as free
                page_tables[i][j].present = 0;
                page_tables[i][j].frame_addr = 0;  
                pmm_free_pages((void *)page_addr, 1, 1); // Free the physical page
            }
            
            if (--pages_to_free == 0) {
                log_step("Virtual region freed successfully.\n", GREEN);
                return;
            }
        }
    }

    log_step("Failed to free virtual region.\n", RED);
}