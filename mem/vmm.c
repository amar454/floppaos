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


typedef struct {
    uintptr_t start_virt; // Start of the virtual address range
    uintptr_t start_phys; // Start of the physical address range
    size_t size;          // Size of the region in bytes
    uint32_t pages;       // Number of pages in the region
    PageAttributes attrs; // Attributes of the pages in the region
} vmm_region;

// Create a new virtual memory region
vmm_region vmm_create_region(PDE *page_directory, uintptr_t start_virt, uintptr_t start_phys, size_t size, PageAttributes attrs) {
    if (!page_directory || size == 0) {
        log("Invalid parameters for creating region!\n", RED);
        return (vmm_region){0};
    }

    // Align size to page size
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    uint32_t pages = size / PAGE_SIZE;

    // Map the region
    for (uint32_t i = 0; i < pages; i++) {
        uintptr_t virt_addr = start_virt + (i * PAGE_SIZE);
        uintptr_t phys_addr = start_phys + (i * PAGE_SIZE);
        vmm_map_page(page_directory, virt_addr, phys_addr, attrs);
    }

    log("Region created successfully.\n", GREEN);
    return (vmm_region){
        .start_virt = start_virt,
        .start_phys = start_phys,
        .size = size,
        .pages = pages,
        .attrs = attrs
    };
}

// Destroy a virtual memory region
void vmm_destroy_region(PDE *page_directory, vmm_region region) {
    if (!page_directory || region.size == 0) {
        log("Invalid parameters for destroying region!\n", RED);
        return;
    }

    // Unmap the region
    for (uint32_t i = 0; i < region.pages; i++) {
        uintptr_t virt_addr = region.start_virt + (i * PAGE_SIZE);
        vmm_unmap_page(page_directory, virt_addr);
    }

    log("Region destroyed successfully.\n", GREEN);
}

void vmm_map_page(PDE *page_directory, uintptr_t virt_addr, uintptr_t phys_addr, PageAttributes attrs) {
    if (!page_directory) {
        log("Invalid page directory!\n", RED);
        return;
    }

    uint32_t dir_idx = virt_addr >> 22;
    uint32_t table_idx = (virt_addr >> 12) & 0x3FF;

    if (!page_directory[dir_idx].present) {
        // Allocate a new page table if not present
        PTE *new_table = (PTE *)pmm_alloc_page();
        if (!new_table) {
            log("Failed to allocate page table!\n", RED);
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
            log("Page already mapped, skipping remap!\n", YELLOW);
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
        log("Invalid page directory!\n", RED);
        return;
    }

    uint32_t dir_idx = virt_addr >> 22;
    uint32_t table_idx = (virt_addr >> 12) & 0x3FF;

    if (!page_directory[dir_idx].present) {
        log("Page directory entry not present!\n", RED);
        return;
    }

    // Unmap the physical page in the page table
    PTE *page_table = (PTE *)((uintptr_t)(page_directory[dir_idx].table_addr << 12));
    if (page_table[table_idx].present) {
        page_table[table_idx].present = 0;
    }
}

void vmm_init() {
    log("Initializing vmm...\n", LIGHT_GRAY);
    flop_memset(page_directory, 0, sizeof(PDE) * PAGE_DIRECTORY_SIZE);  // Initialize the page directory
    log("VMM initialized.\n", GREEN);
}

void* vmm_malloc(uint32_t size) {
    log("Allocating virtual memory...\n", WHITE);

    if (size == 0) return NULL;

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
                    // Allocate and map physical pages
                    for (uint32_t k = 0; k < pages_needed; k++) {
                        uintptr_t virt_addr = start_virt + (k * PAGE_SIZE);
                        void* phys_addr = pmm_alloc_page();
                        if (!phys_addr) {
                            log("Failed to allocate physical memory!\n", RED);

                            // Cleanup already allocated pages
                            for (uint32_t l = 0; l < k; l++) {
                                uintptr_t cleanup_virt_addr = start_virt + (l * PAGE_SIZE);
                                vmm_unmap_page(page_directory, cleanup_virt_addr);
                            }
                            return NULL;
                        }

                        vmm_map_page(page_directory, virt_addr, (uintptr_t)phys_addr, (PageAttributes){
                            .present = 1,
                            .rw = 1,
                            .user = 1
                        });
                    }

                    // Create a new region for the allocation
                    vmm_region region = vmm_create_region(page_directory, start_virt, 0, size, (PageAttributes){
                        .present = 1,
                        .rw = 1,
                        .user = 1
                    });

                    log("Virtual memory allocated successfully.\n", GREEN);
                    return (void*)region.start_virt;
                }
            } else {
                found_pages = 0;
            }
        }
    }

    log("Virtual memory allocation failed.\n", RED);
    return NULL;
}
// Free virtual memory and unmap physical pages
void vmm_free(void* virt_addr, uint32_t size) {
    if (!virt_addr || size == 0) return;

    // Align size to page size
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    // Find the region corresponding to the virtual address
    uintptr_t start_virt = (uintptr_t)virt_addr;
    vmm_region region = vmm_create_region(page_directory, start_virt, 0, size, (PageAttributes){0});

    if (region.size == 0) {
        log("vmm_free: Region not found!\n", RED);
        return;
    }

    // Unmap and free the region
    vmm_destroy_region(page_directory, region);

    log("Virtual memory freed.\n", GREEN);
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
        log("Invalid parameters for struct region mapping!\n", RED);
        return NULL;
    }

    // Calculate number of pages needed for the struct
    uint32_t pages_needed = (struct_size + PAGE_SIZE - 1) / PAGE_SIZE;
    
    // Allocate virtual memory for the struct
    void *struct_region = vmm_malloc(pages_needed * PAGE_SIZE);
    if (!struct_region) {
        log("Failed to allocate struct region!\n", RED);
        return NULL;
    }

    // Initialize the memory region to zero
    flop_memset(struct_region, 0, pages_needed * PAGE_SIZE);
    
    log("Struct region mapped successfully.\n", GREEN);
    return struct_region;
}

void vmm_unmap_struct_region(PDE *page_directory, void *struct_region, size_t struct_size) {
    if (!page_directory || !struct_region) {
        log("Invalid parameters for struct region unmapping!\n", RED);
        return;
    }

    // Calculate number of pages needed for the struct
    uint32_t pages_needed = (struct_size + PAGE_SIZE - 1) / PAGE_SIZE;
    
    // Free the virtual memory for the struct
    vmm_free(struct_region, pages_needed * PAGE_SIZE);

    log("Struct region unmapped successfully.\n", GREEN);
}

void *vmm_reserve_region(PDE *page_directory, size_t size) {
    if (!page_directory || size == 0) {
        log("Invalid parameters for reserving region!\n", RED);
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
                    log("Virtual region reserved successfully.\n", GREEN);
                    return (void *)start_virt;
                }
            } else {
                found_pages = 0;
            }
        }
    }

    log("Failed to reserve virtual region.\n", RED);
    return NULL;
}


void vmm_free_region(PDE *page_directory, void *virt_addr, size_t size) {
    if (!page_directory || !virt_addr || size == 0) {
        log("Invalid parameters for freeing region!\n", RED);
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
                log("Virtual region freed successfully.\n", GREEN);
                return;
            }
        }
    }

    log("Failed to free virtual region.\n", RED);
}

void test_vmm() {
    log("Testing vmm_malloc and vmm_free...\n", LIGHT_GRAY);

    // Allocate 16 KB of virtual memory
    void* addr = vmm_malloc(16 * 1024);
    if (addr) {
        log_address("Allocated virtual memory at: ", (uint32_t)addr);

        // Free the allocated memory
        vmm_free(addr, 16 * 1024);
        log("Freed virtual memory.\n", GREEN);
    } else {
        log("Failed to allocate virtual memory.\n", RED);
    }
}