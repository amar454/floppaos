#include <stdint.h>
#include "pmm.h"
#include "utils.h"
#include "vmm.h"
#include "../apps/echo.h"
#include "../drivers/vga/vgahandler.h"
#include "../lib/logging.h"
// Define page directory and page tables
PDE page_directory[PAGE_DIRECTORY_SIZE] __attribute__((aligned(PAGE_SIZE)));
PTE page_tables[PAGE_DIRECTORY_SIZE][PAGE_TABLE_SIZE] __attribute__((aligned(PAGE_SIZE)));


// Set page table entry flags using a PageAttributes struct
void set_page_flags(PTE *entry, PageAttributes attributes) {
    entry->present = attributes.present;
    entry->rw = attributes.rw;
    entry->user = attributes.user;
    entry->write_thru = attributes.write_thru;
    entry->cache_dis = attributes.cache_dis;
    entry->accessed = attributes.accessed;
    entry->dirty = attributes.dirty;
    entry->pat = attributes.pat;
    entry->global = attributes.global;
    entry->available = attributes.available;
    entry->frame_addr = attributes.frame_addr;
}

void enable_paging(uint8_t enable_wp, uint8_t enable_pse) {
    uint32_t cr0, cr4;

    log_step("Enabling paging...\n", LIGHT_GRAY);

    // Log the base address of the page directory being loaded into CR3
    log_step("Loading page directory base address into CR3...\n", LIGHT_BLUE);
    log_step("Page Directory Base Address: ", LIGHT_GRAY);
    log_address("", (uintptr_t)page_directory);

    // Load the page directory base address into CR3
    __asm__ volatile("mov %0, %%cr3" : : "r"(page_directory));

    // Enable paging by setting the PG bit (bit 31) and optionally WP (bit 16) in CR0
    log_step("Modifying CR0 to enable paging...\n", LIGHT_BLUE);
    log_step("Setting PG (Paging Enable) (bit 31)  bit...\n", LIGHT_BLUE);
    if (enable_wp) {
        log_step("Enabling WP (Write Protect) (bit 31) in CR0...\n", LIGHT_BLUE);
    }
    __asm__ volatile(
        "mov %%cr0, %0\n"
        "or %1, %0\n"
        "mov %0, %%cr0"
        : "=r"(cr0) 
        : "r"(enable_wp ? 0x80010000 : 0x80000000) // 0x80010000 -> PG + WP
    );
    log_step("CR0 modified successfully.\n", LIGHT_GREEN);
    // Enable PSE (Page Size Extension) if requested by setting the PSE bit (bit 4) in CR4
    if (enable_pse) {
        log_step("Enabling PSE (Page Size Extension) (bit 4) in CR4...\n", LIGHT_BLUE);
        __asm__ volatile(
            "mov %%cr0, %0\n"
            "or %1, %0\n"
            "mov %0, %%cr0"
            : "=r"(cr0)
            : "r"(enable_wp ? 0x80010000 : 0x80000000)
            : "memory"
        );
        

        // Log the updated CR4 value
        log_step("PSE enabled successfully in CR4.\n", LIGHT_GREEN);
    } else {
        log_step("Skipping PSE (Page Size Extension) setup as requested.\n", LIGHT_BLUE);
    }

    // Final confirmation
    log_step("Paging successfully enabled.\n", GREEN);
}


void create_first_page_table() {
    log_step("Creating first page table (identity-mapped 4 MiB)...\n", LIGHT_GRAY);

    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        uint32_t frame = i * PAGE_SIZE; // Physical address
        PageAttributes attributes = {
            .present = 1,
            .rw = 1,
            .user = 0,
            .write_thru = 0,
            .cache_dis = 0,
            .accessed = 0,
            .dirty = 0,
            .pat = 0,
            .global = 0,
            .available = 0,
            .frame_addr = frame >> 12
        };

        set_page_flags(&page_tables[0][i], attributes);

        if (i % 64 == 0) { // Reduce log spam
            log_step("Mapped frame: ", LIGHT_GRAY);
            log_address("", frame);
        }
    }
    log_step("First page table created.\n", GREEN);
}


// Create the page directory and link the first page table
void create_page_directory() {
    log_step("Creating page directory...\n", LIGHT_GRAY);

    log_step("Initializing page directory entries...\n", LIGHT_GRAY);
    for (int i = 0; i < PAGE_DIRECTORY_SIZE; i++) {
        page_directory[i].present = 0;
        page_directory[i].rw = 1;   // Read/Write
        page_directory[i].user = 0; // Supervisor (kernel mode)
        page_directory[i].table_addr = ((uintptr_t)&page_tables[i] >> 12) & 0xFFFFF;

    }
    
    // Link the first page table
    log_step("Linking the first page table...\n", LIGHT_GRAY);
    page_directory[0].present = 1; // Mark the first entry as present
    
    log_step("Page directory created and first table linked.\n", GREEN);
}

// Initialize paging
void paging_init() {

    log_step("Initializing paging... \n ", LIGHT_GRAY);

    create_page_directory();  // Create and initialize the page directory
    create_first_page_table(); // Identity map the first 4 MiB
    enable_paging(1, 1);      // Enable paging with WP and PSE
    log_step("Paging initialized. \n ", GREEN);

}
