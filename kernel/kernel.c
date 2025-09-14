/* 

Copyright 2024, 2025 Amar Djulovic <aaamargml@gmail.com>

This file is part of FloppaOS.

FloppaOS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

FloppaOS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with FloppaOS. If not, see <https://www.gnu.org/licenses/>.

------------------------------------------------------------------------------

kernel.c:

    This is the kernel of floppaOS, a free and open-source 32-bit operating system.
    This is the prerelease alpha-v0.1.3 code, still in development.
    It is multiboot compliant, and takes the multiboot info pointer and magic number in kmain. 
    
    kmain(...) called by the boot.asm file and linked to it by the linker.ld file.
    
    panic(...) is used across the kernel for errors that require a restart.
    
    halt() uses an infinite while-loop to halt in C. It's not a true cpu halt but it's mainly just for isolating parts of the kernel for debugging


------------------------------------------------------------------------------
*/

#include "kernel.h"
#include "../apps/echo.h"
#include "../drivers/time/floptime.h"
#include "../fs/tmpflopfs/tmpflopfs.h"
#include "../fs/vfs/vfs.h"
#include "../drivers/keyboard/keyboard.h"
#include "../interrupts/interrupts.h"
#include "../lib/str.h"
#include "../lib/assert.h"
#include "../mem/pmm.h"
#include "../mem/utils.h"
#include "../mem/slab.h"
#include "../mem/vmm.h"
#include "../mem/gdt.h"
#include "../mem/alloc.h"
#include "../task/sched.h"
#include "../task/thread.h"
#include "../drivers/vga/vgahandler.h"
#include "../mem/paging.h"
#include "../lib/logging.h"
#include "../multiboot/multiboot.h"
#include "../drivers/vga/framebuffer.h"
#include <stdint.h>
#include "../flanterm/src/flanterm.h"
#include "../flanterm/src/flanterm_backends/fb.h"


 void halt() { 
    while (1) {
        continue;
    }
}

 void cpuhalt() {
    __asm__ volatile("hlt");
}

void panic(uint32_t address, const char* msg, const char* err) {

    
    halt();

}

void mem_dump(uint32_t address, uint32_t length) {
    uint32_t *ptr = (uint32_t *)(uintptr_t)address;    
    for (uint32_t i = 0; i < length; i++) {
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

static void check_multiboot_magic(uint32_t magic) {
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        vga_place_string(0, 0,"Multiboot magic number incorrect!", RED);
        halt();
    }
    vga_place_string(0, 0,"Multiboot magic number correct!", GREEN);
}
void fshell_task(void *arg);
static void check_multiboot_info(multiboot_info_t *mb_info) {
    if (!mb_info) {
        vga_place_string(0, 0,"Multiboot info pointer is NULL!", RED);
        halt();
    }
    vga_place_string(0, 0,"Multiboot info pointer is correct!", GREEN);
    if (mb_info->flags & MULTIBOOT_INFO_MEMORY) {
        vga_place_string(0, 0,"Multiboot info pointer has memory information!", GREEN);
    } else {
        vga_place_string(0, 0,"Multiboot info pointer does not have memory information!", RED);
        halt();
        
    }

}

#define VERSION "0.1.3-alpha"
int kmain(uint32_t magic, multiboot_info_t *mb_info) {
    framebuffer_init(mb_info); 
    init_console();
    log("floppaOS kernel framebuffer init - ok\n", GREEN);
    log("floppaOS - The Floperrating system, a free and open-source 32-bit hobby operating system\n", YELLOW);
    
    log("Kernel compilation time: " __DATE__ " at " __TIME__ "\n", YELLOW);
    log("License: GPLv3\n", YELLOW);
    log("Date created: October 2024\n", YELLOW);
    log("Author: Amar Djulovic <aaamargml@gmail.com>\n", YELLOW);
    log("Kernel version: " VERSION "\n", YELLOW);
    log("Starting floppaOS kernel...\n", YELLOW);
    gdt_init(); 
    init_interrupts(); 

    pmm_init(mb_info);
    paging_init(); 

    slab_init(); 

   
    vmm_init(); 
    init_kernel_heap();

    vfs_init();

    tmpfs_init();

    sched_init();

    echo("floppaOS kernel booted! now we do nothing.\n", GREEN);
    draw_floppaos_logo(); 
    echo ("floppaOS, The Flopperating System - Copyright (C) 2024, 2025 Amar Djulovic <aaamargml@gmail.com>\n", YELLOW); // copyright notice
  
    

    while (1) {
    }
    return 0;
    
}
int main() {
    return 0;
}
