#include "gdt.h"
#include "utils.h"
#include <stdint.h>
#include "../apps/echo.h"
#include "../drivers/vga/vgahandler.h"
#include "../lib/logging.h"
// Define the Global Descriptor Table (GDT) and its pointer
#define GDT_ENTRIES 3
GDTEntry gdt[GDT_ENTRIES];
GDTPointer gdt_ptr;

// Inline assembly function to flush the GDT
void gdt_flush(uint32_t gdt_ptr_address) {
    __asm__ __volatile__ (
        "lgdt (%0)\n"          
        "mov $0x10, %%ax\n"   
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        "jmp $0x08, $.next\n" 
        ".next:\n"
        :                     
        : "r" (gdt_ptr_address) //
        : "eax"               
    );
}

// Function to set an entry in the GDT
void set_gdt_entry(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity) {

    gdt[num].base_low = (uint16_t)(base & 0xFFFF);
    gdt[num].base_middle = (uint8_t)((base >> 16) & 0xFF);
    gdt[num].base_high = (uint8_t)((base >> 24) & 0xFF);

    gdt[num].limit_low = (uint16_t)(limit & 0xFFFF);
    gdt[num].granularity = (uint8_t)(((limit >> 16) & 0x0F) | (granularity & 0xF0));
    gdt[num].access = access;
}

void init_gdt() {
    gdt_ptr.limit = (sizeof(GDTEntry) * GDT_ENTRIES) - 1;
    gdt_ptr.base = (uintptr_t)&gdt; // Explicit cast
    echo("[mem/gdt.c]\n", WHITE);
    log_step("Initializing gdt... ", LIGHT_GRAY);
    log_step("Setting null descriptor entry...\n", LIGHT_GRAY);
    set_gdt_entry(0, 0, 0, 0, 0);               // Null Descriptor
    
    log_step("Setting code segment entry...\n", LIGHT_GRAY);
    set_gdt_entry(1, 0, 0xFFFFF, 0x9A, 0xCF);   // Code Segment
    log_step("Setting data segment entry...\n", LIGHT_GRAY);
    set_gdt_entry(2, 0, 0xFFFFF, 0x92, 0xCF);   // Data Segment

    log_step("Flushing gdt...\n", LIGHT_GRAY);
    gdt_flush((uint32_t)(uintptr_t)&gdt_ptr); // Correctly cast the pointer

    log_step("Gdt initialized. \n ", GREEN);
}

