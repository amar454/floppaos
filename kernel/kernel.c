/* 

Copyright 2024, 2025 Amar Djulovic <aaamargml@gmail.com>

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
#include "../apps/echo.h"
#include "../drivers/time/floptime.h"
#include "../fs/tmpflopfs/tmpflopfs.h"
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
#include "../flanterm/flanterm.h"
#include "../flanterm/backends/fb.h"


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
int kmain(uint32_t magic, multiboot_info_t *mb_info) {
    vga_place_string(0, 0, "early_init: floppa os booting...", WHITE);
    sleep_seconds(1);
    check_multiboot_magic(magic);
    check_multiboot_info(mb_info);
    framebuffer_init(mb_info); 
    init_console();
    sleep_seconds(1);
    init_gdt(); // :meme:
    init_interrupts(); 
    IA32_INT_UNMASK(); 
    sleep_seconds(1);
    pmm_init(mb_info);
    slab_init(); 
    sleep_seconds(1);
    paging_init(); 
    vmm_init(); 
    init_kernel_heap();

    sleep_seconds(1);
    tmpfs_init();

    sched_init();
    sleep_seconds(1);
  
    sched_start();
    draw_floppaos_logo(); 
    echo ("Flopperating System - Copyright (C) 2024, 2025 Amar Djulovic <aaamargml@gmail.com>\n", YELLOW); // copyright notice
  
    

    while (1) {
    }
    return 0;
    
}
// bullshit placeholder bc C requires a main method but doesn't take the correct types of the magic number and info pointer in eax and ebx
int main() {
    return 0;
}
