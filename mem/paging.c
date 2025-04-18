#include <stdint.h>
#include "pmm.h"
#include "utils.h"
#include "vmm.h"
#include "paging.h"
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

    log("paging: Enabling paging...\n", LIGHT_GRAY);

    // Log the base address of the page directory being loaded into CR3
    log("paging: Loading page directory base address into CR3...\n", LIGHT_BLUE);
    log("paging: Page Directory Base Address: ", LIGHT_GRAY);
    log_address("", (uintptr_t)page_directory);

    // Load the page directory base address into CR3
    __asm__ volatile("mov %0, %%cr3" : : "r"(page_directory));

    // Enable paging by setting the PG bit (bit 31) and optionally WP (bit 16) in CR0
    log("paging: Modifying CR0 to enable paging...\n", LIGHT_BLUE);
    log("paging: Setting PG (Paging Enable) (bit 31)  bit...\n", LIGHT_BLUE);
    if (enable_wp) {
        log("paging: Enabling WP (Write Protect) (bit 31) in CR0...\n", LIGHT_BLUE);
    }
    __asm__ volatile(
        "mov %%cr0, %0\n"
        "or %1, %0\n"
        "mov %0, %%cr0"
        : "=r"(cr0) 
        : "r"(enable_wp ? 0x80010000 : 0x80000000) // 0x80010000 -> PG + WP
    );
    log("paging: CR0 modified successfully.\n", LIGHT_GREEN);
    // Enable PSE (Page Size Extension) if requested by setting the PSE bit (bit 4) in CR4
    if (enable_pse) {
        log("paging: Enabling PSE (Page Size Extension) (bit 4) in CR4...\n", LIGHT_BLUE);
        __asm__ volatile(
            "mov %%cr0, %0\n"
            "or %1, %0\n"
            "mov %0, %%cr0"
            : "=r"(cr0)
            : "r"(enable_wp ? 0x80010000 : 0x80000000)
            : "memory"
        );
        

        // Log the updated CR4 value
        log("paging: PSE enabled successfully in CR4.\n", LIGHT_GREEN);
    } else {
        log("paging: Skipping PSE (Page Size Extension) setup as requested.\n", LIGHT_BLUE);
    }

    // Final confirmation
    log("paging: Paging successfully enabled.\n", GREEN);
}


static void create_first_page_table() {
    log("paging: Creating first page table (identity-mapped 4 MiB)...\n", LIGHT_GRAY);

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
            log("paging: Mapped frame: ", LIGHT_GRAY);
            log_address("", frame);
        }
    }
    log("paging: First page table created.\n", GREEN);
}


// Create the page directory and link the first page table
static void create_first_page_directory() {
    log("paging: Creating page directory...\n", LIGHT_GRAY);

    log("paging: Initializing page directory entries...\n", LIGHT_GRAY);
    for (int i = 0; i < PAGE_DIRECTORY_SIZE; i++) {
        PageAttributes attributes = {
            .present = 0,
            .rw = 1,   // Read/Write
            .user = 0, // Supervisor (kernel mode)
            .write_thru = 0,
            .cache_dis = 0,
            .accessed = 0,
            .dirty = 0,
            .pat = 0,
            .global = 0,
            .available = 0,
            .frame_addr = 0
        };
        set_page_flags((PTE *)&page_directory[i], attributes);
    }

    // Link the first page table
    log("paging: Linking the first page table...\n", LIGHT_GRAY);
    page_directory[0].present = 1; // Mark the first entry as present
    page_directory[0].rw = 1;      // Read/Write
    page_directory[0].user = 0;   // Supervisor (kernel mode)
    page_directory[0].table_addr = ((uintptr_t)&page_tables[0] >> 12);

    log("paging: Page directory created and first table linked.\n", GREEN);
}

// Create a new page directory
PDE *create_new_page_directory() {
    log("paging: Creating a new page directory...\n", LIGHT_GRAY);

    PDE *new_page_directory = (PDE *)pmm_alloc_page();
    if (!new_page_directory) {
        log("paging: Failed to allocate memory for new page directory!\n", RED);
        return NULL;
    }

    log("paging: Initializing new page directory entries...\n", LIGHT_GRAY);
    for (int i = 0; i < PAGE_DIRECTORY_SIZE; i++) {
        PageAttributes attributes = {
            .present = 0,
            .rw = 1,   // Read/Write
            .user = 0, // Supervisor (kernel mode)
            .write_thru = 0,
            .cache_dis = 0,
            .accessed = 0,
            .dirty = 0,
            .pat = 0,
            .global = 0,
            .available = 0,
            .frame_addr = 0
        };
        set_page_flags((PTE *)&new_page_directory[i], attributes);
    }

    log("paging: New page directory created successfully.\n", GREEN);
    return new_page_directory;
}

PDE *create_page_directory() {
    PDE *page_directory = (PDE *)pmm_alloc_page();
    if (!page_directory) {
        log("paging: Failed to allocate page directory!\n", RED);
        return NULL;
    }

    PageAttributes attributes = {
        .present = 0,
        .rw = 0,
        .user = 0,
        .write_thru = 0,
        .cache_dis = 0,
        .accessed = 0,
        .dirty = 0,
        .pat = 0,
        .global = 0,
        .available = 0,
        .frame_addr = 0
    };

    // Initialize all entries in the page directory
    for (int i = 0; i < PAGE_DIRECTORY_SIZE; i++) {
        set_page_flags((PTE *)&page_directory[i], attributes);
    }

    return page_directory;
}

PDE *create_user_page_directory() { 
    PDE *user_page_directory = (PDE *)pmm_alloc_page();
    if (!user_page_directory) {
        log("paging: Failed to allocate user page directory!\n", RED);
        return NULL;
    }

    PageAttributes attributes = {
        .present = 0,
        .rw = 1,       // Read/Write
        .user = 1,     // User mode
        .write_thru = 0,
        .cache_dis = 0,
        .accessed = 0,
        .dirty = 0,
        .pat = 0,
        .global = 0,
        .available = 0,
        .frame_addr = 0
    };

    for (int i = 0; i < PAGE_DIRECTORY_SIZE; i++) {
        set_page_flags((PTE *)&user_page_directory[i], attributes);
    }

    return user_page_directory;
}

PDE *destroy_user_page_directory(PDE *user_page_directory) {
    if (!user_page_directory) return NULL;

    for (int i = 0; i < PAGE_DIRECTORY_SIZE; i++) {
        if (user_page_directory[i].present) {
            PDE *page_table = (PDE *)(uintptr_t)(user_page_directory[i].table_addr << 12);
            pmm_free_page(page_table);
        }
    }

    pmm_free_page(user_page_directory);
    return NULL;
}

PTE *get_page_table(PDE *page_directory, uint32_t index) {
    if (index >= PAGE_DIRECTORY_SIZE) {
        log("paging: Invalid page directory index!\n", RED);
        return NULL;
    }

    if (!page_directory[index].present) {
        log_uint("Page table not present at index: ", index);
        return NULL;
    }

    return (PTE *)(uintptr_t)(page_directory[index].table_addr << 12);
}

PTE *get_page_table_entry(PDE *page_directory, uintptr_t virt_addr) {
    uint32_t dir_index = virt_addr >> 22;
    uint32_t table_index = (virt_addr >> 12) & 0x3FF;

    if (!page_directory[dir_index].present) {
        log("paging: Page directory entry not present!\n", RED);
        return NULL;
    }

    PTE *page_table = get_page_table(page_directory, dir_index);
    if (!page_table) {
        log("paging: Failed to get page table!\n", RED);
        return NULL;
    }

    return &page_table[table_index];
}

PTE *get_page_table_entry_by_index(PDE *page_directory, uint32_t index) {
    if (index >= PAGE_DIRECTORY_SIZE) {
        log("paging: Invalid page directory index!\n", RED);
        return NULL;
    }

    if (!page_directory[index].present) {
        log_uint("paging: Page table not present at index: \n", index);
        return NULL;
    }

    return (PTE *)(uintptr_t)(page_directory[index].table_addr << 12);
}

PTE *destroy_page_table(PDE *page_directory, uint32_t index) {
    if (index >= PAGE_DIRECTORY_SIZE) {
        log("paging: Invalid page directory index!\n", RED);
        return NULL;
    }

    if (!page_directory[index].present) {
        log_uint("paging: Page table not present at index: ", index);
        return NULL;
    }

    PTE *page_table = get_page_table(page_directory, index);
    if (!page_table) {
        log("paging: Failed to get page table!\n", RED);
        return NULL;
    }

    pmm_free_page(page_table);
    page_directory[index].present = 0;

    return NULL;
}

void destroy_page_directory(PDE *page_directory) {
    if (!page_directory) return;

    for (int i = 0; i < PAGE_DIRECTORY_SIZE; i++) {
        if (page_directory[i].present) {
            PDE *page_table = (PDE *)(uintptr_t)(page_directory[i].table_addr << 12);
            pmm_free_page(page_table);
        }
    }

    pmm_free_page(page_directory);
}



// Initialize paging
void paging_init() {

    log("paging: Initializing paging... \n ", LIGHT_GRAY);
    create_first_page_directory();  // Create and initialize the page directory
    create_first_page_table(); // Identity map the first 4 MiB
    enable_paging(1, 1);      // Enable paging with WP and PSE
    log("paging: Paging initialized. \n ", GREEN);

}
