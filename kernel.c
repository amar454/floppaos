/* 

Copyright 2024-25 Amar Djulovic <aaamargml@gmail.com>

This file is part of FloppaOS.

FloppaOS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

FloppaOS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with FloppaOS. If not, see <https://www.gnu.org/licenses/>.

------------------------------------------------------------------------------

kernel.c:

    This is the kernel of floppaOS, a free and open-source 32-bit operating system.
    This is the prerelease alpha-v0.1.2 code, still in development.
    It is multiboot compliant, and takes the multiboot info pointer and magic number in kmain. 
    
    kmain(...) called by the boot.asm file and linked to it by the linker.ld file.
    
    panic(...) is used across the kernel for errors that require a restart.
    
    halt() uses an infinite while-loop to halt in C. It's not a true cpu halt but it's mainly just for isolating parts of the kernel for debugging


------------------------------------------------------------------------------
*/

#include "kernel.h"
#include "apps/echo.h"
#include "drivers/time/floptime.h"
#include "fs/tmpflopfs/tmpflopfs.h"
#include "fshell/fshell.h"
#include "drivers/keyboard/keyboard.h"
#include "interrupts/interrupts.h"
#include "lib/str.h"
#include "mem/pmm.h"
#include "mem/utils.h"
#include "mem/slab.h"
#include "mem/vmm.h"
#include "mem/gdt.h"
#include "mem/alloc.h"
#include "task/task_handler.h"
#include "drivers/vga/vgahandler.h"
#include "mem/paging.h"
#include "lib/logging.h"
#include "multiboot/multiboot.h"
#include "drivers/vga/framebuffer.h"
#include <stdint.h>

void halt() { 
    while (1) {
        continue;
    }
}


void cpuhalt() {
    __asm__ volatile("hlt");
}

void panic(uint32_t address, const char* msg, const char* err) {
    // Clear the screen first
    vga_clear_screen();

    // Calculate dimensions for centered box
    uint16_t width = 40;
    uint16_t height = 8;
    uint16_t x_start = (80 - width) / 2;  // Center horizontally
    uint16_t y_start = (25 - height) / 2;  // Center vertically

    // Draw the main box
    draw_box(x_start, y_start, width, height, "KERNEL PANIC ERROR", RED, RED);

    // Format address string
    char addr_str[32];
    flopsnprintf(addr_str, sizeof(addr_str), "Address: 0x%08x", address);
    
    // Place content inside box
    vga_place_string(x_start + 2, y_start + 3, addr_str, RED);
    vga_place_string(x_start + 2, y_start + 4, "Message: ", RED);
    vga_place_string(x_start + 10, y_start + 4, msg, LIGHT_RED);
    vga_place_string(x_start + 2, y_start + 5, "Type: ", RED);
    vga_place_string(x_start + 8, y_start + 5, err, LIGHT_RED);
    vga_place_string(x_start + 2, y_start + 6, "Contact: aaamargml@gmail.com", YELLOW);

    halt();

}

void mem_dump(uint32_t address, uint32_t length) {
    uint32_t *ptr = (uint32_t *)(uintptr_t)address;    for (uint32_t i = 0; i < length; i++) {
        if (i % 16 == 0) {
            echo("\n", WHITE);
        }
        log_address("", ptr[i]);
    }
    echo("\n", WHITE);
}

void draw_floppaos_logo() {
    const char *ascii_art = 
    "  __ _                          ___  ____   \n"
    " / _| | ___  _ __  _ __   __ _ / _ \\/ ___|  \n"
    "| |_| |/ _ \\| '_ \\| '_ \\ / _` | | | \\___ \\  \n"
    "|  _| | (_) | |_) | |_) | (_| | |_| |___) | \n"
    "|_| |_|\\___/| .__/| .__/ \\__,_|\\___/|____/ v0.1.1-alpha \n"
    "            |_|   |_|                      \n";
    echo(ascii_art, YELLOW);
    sleep_seconds(1);
}

// cute helper function to check if the multiboot magic number is valid
static void check_multiboot_magic(uint32_t magic) {
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        panic(0, "Multiboot magic number mismatch!", "System halted.");
        halt();
    }
}

// cute helper function to check if the multiboot info structure is valid
static void check_multiboot_info(multiboot_info_t *mb_info) {
    if (!mb_info) {
        panic((uintptr_t)mb_info, "The multiboot info structure could not be found!", "MultibootInfoNullError");
        
    }

}
/**
 * @name kmain 
 * @brief Main kernel entry point
 * @param magic Multiboot magic number
 * @param mb_info Multiboot information structure
 */
int kmain(uint32_t magic, multiboot_info_t *mb_info) {
    echo("Booting floppaOS alpha v0.0.2-alpha...\n", WHITE);

    // check if multiboot magic and info exist and are valid
    check_multiboot_magic(magic);
    check_multiboot_info(mb_info);
    
    // Print Multiboot information
    print_multiboot_info(mb_info);
    sleep_seconds(1);

    init_gdt(); // well well well


    // initialize memory
    pmm_init(mb_info); // now we can alloc pages
    slab_init(); // now we can use slabs to allocate smaller sizes
    paging_init(); // yay now paging works (has to be enabled)
    vmm_init(); // we can now make virtual address spaces and reserve regions
    init_kernel_heap(); // now we can use pmm and slab for allocating in the kernel



    // init interrupts
    // pic, pit, isr, and idt routines initialized here.
    init_interrupts(); 
    
    __asm__ volatile("sti"); // yay we have interrupts now

    // init scheduler
    sched_init(); 

    // init file system
    struct TmpFileSystem tmp_fs;
    init_tmpflopfs(&tmp_fs);  
    
    // add tasks (fshell and keyboard)
    add_task(fshell_task, &tmp_fs, 0, "fshell", "floppaos://fshell/fshell.c");  
    add_task(keyboard_task, NULL, 1, "keyboard", "floppaos://drivers/keyboard/keyboard.c"); // mainly here for fun
    
    // make time struct and add task
    struct Time system_time; 
    add_task(time_task, &system_time, 2, "floptime", "floppaos://drivers/time/floptime.c"); // cool little time display
    
    // print cool stuff
    draw_floppaos_logo(); 
    echo ("Welcome to floppaOS!\n", WHITE);
    echo ("floppaOS - Copyright (C) 2024-25 Amar Djulovic <aaamargml@gmail.com>\n", YELLOW); // copyright notice

    // start scheduler
    sched_start();
    
    // oh no the os has stopped scheduling


    do {
        halt(); // we're done here, halt the cpu.
        
        // see you in the next life 
    } while (1);
    
}

// bullshit placeholder bc C requires a main method but doesn't take the correct types of the magic number and info pointer in eax and ebx
int main() {
    return 0;
}
