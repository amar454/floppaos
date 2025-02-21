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
#include "mem/pmm.h"
#include "mem/vmm.h"
#include "mem/utils.h"
#include "mem/gdt.h"
#include "mem/kalloc.h"
#include "task/task_handler.h"
#include "drivers/vga/vgahandler.h"
#include "mem/paging.h"
#include "lib/logging.h"
#include "lib/str.h"
#include "multiboot/multiboot.h"
#include "drivers/vga/framebuffer.h"
#include "interrupts/interrupts.h"

int time_ready = 1; 

/**
 * Halts the system.
 * 
 * @note This function does not return.
 * @warning This function leads to an unusasble state and should only be used for debugging.
 * @warning This function should only be used in case of an unrecoverable error.
 * @warning A manual restart is required after this function is called.
 * @warning Not a true CPU halt; It is a software halt that calls an infinite loop in C.
 * @note Consider using __asm__ volatile ("hlt") for a true CPU halt.
 */
void halt() { 
    while (1) {
        continue;
    }
}

/**
 * Prints a kernel panic message and halts the system.
 * 
 * @param address The address where the panic occurred.
 * @param msg The panic message.
 * @param err The error message.
 * @note This function does not return.
 * @warning This function should only be used in case of a kernel panic.
 * @warning This function will not halt the system.
 */ 
void panic(uint32_t address, const char* msg, const char* err) {  
    log_step("KERNEL PANIC ***********************\n:(\n", LIGHT_RED);
    for(int i = 1; i > 6; i++) {
        echo("***********************\n", LIGHT_RED);
    }
    
    log_step("floppaOS kernel has reached a panic state and needs to be restarted.\n", LIGHT_RED);
    log_address("", address);
    log_step("Error name ", RED);

    log_step(msg, YELLOW);
    
}

/**
 * Memory dump function.
 * 
 * @author Amar Djulovic
 * 
 * @param start The start address of the memory dump.
 * @param end The end address of the memory dump.
 * 
 * @note This function should be called after a kernel panic to dump memory.
 * @note This function is used for debugging purposes, and should be overlooked be the user for the most part.
 * 
 * @warning This dunction is memory unsafe and should be used with caution.
 * @warning This function can easily fill the whole screen with memory addresses.
 * 
 */
void mem_dump(uint32_t start, uint32_t end) {
    uint32_t *ptr = (uint32_t *)start;
    while ((uint32_t)ptr < end) {
        log_address("%x ", *ptr);
        ptr++;
    }
}



void draw_floppaos_logo() {
    const char *ascii_art = 
    "  __ _                          ___  ____   \n"
    " / _| | ___  _ __  _ __   __ _ / _ \\/ ___|  \n"
    "| |_| |/ _ \\| '_ \\| '_ \\ / _` | | | \\___ \\  \n"
    "|  _| | (_) | |_) | |_) | (_| | |_| |___) | \n"
    "|_| |_|\\___/| .__/| .__/ \\__,_|\\___/|____/ v0.1.1-alpha \n"
    "            |_|   |_|                      \n";
    char *ascii_art_mem = (char *)kmalloc(flopstrlen(ascii_art) + 1);
    if (!ascii_art_mem) {
        panic(0, "Failed to allocate memory for ASCII art!", "System halted.");
        halt();
    }
    flopstrcopy(ascii_art_mem, ascii_art, flopstrlen(ascii_art) + 1);
    echo(ascii_art_mem, YELLOW);
    kfree(ascii_art_mem);
    sleep_seconds(1);
    echo(ascii_art, YELLOW); // print cool stuff
}


void verify_boot_magic(uint32_t magic) {
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        panic((uint32_t)&magic, "Invalid Multiboot magic number!", "System halted.");
        halt();
    }
    log_step("Valid Multiboot magic number detected.", GREEN);
    log_address("Magic number: ", magic);
}

/**
 * The main function of the kernel.
 * 
 * @param magic The magic number passed by the bootloader.
 * @param mb_info The multiboot information structure.
 * 
 * @note The magic number and multiboot info structure are passed by the bootloader.
 * @note They are pushed to eax and ebx registers respectively by the boot.asm file.
 * 
 * @note The order in which they are pushed proceeds as follows:
 * @note 1. The magic number is pushed to eax.
 * @note 2. The multiboot info structure is pushed to ebx.
 * @note 3. The kernel is called with the kmain function.
 * @note 4. The kernel checks the magic number and multiboot info structure.
 * @note 5. The kernel initializes the memory management system.
 * @note 6. The kernel initializes the task system.
 * @note 7. The kernel initializes the keyboard.
 * @note 8. The kernel initializes the interrupts.
 * @note 9. The kernel enables interrupts.
 * @note 10. The kernel prints the floppaOS logo.
 * @note 11. The kernel prints the copyright notice.
 * @note 12. The kernel starts the scheduler.
 * 
 * @return 0 on success.
 * @note This function should never return, as the system should be halted in case of an error.
 */
int kmain(uint32_t magic, multiboot_info_t *mb_info) {
    echo("Booting floppaOS alpha v0.0.2-alpha...\n", WHITE);

    verify_boot_magic(magic);
    print_multiboot_info(mb_info);
    sleep_seconds(3);

    // initialize memory
    init_gdt();
    pmm_init(mb_info);
    sleep_seconds(1);
    paging_init();
    sleep_seconds(1);
    vmm_init();
    sleep_seconds(1);

    struct TmpFileSystem tmp_fs;
    init_tmpflopfs(&tmp_fs);  

    initialize_task_system();
    
    add_task(fshell_task, &tmp_fs, 0, "fshell", "floppaos://fshell/fshell.c");  
    struct Time system_time; 
    add_task(time_task, &system_time, 2, "floptime", "floppaos://drivers/time/floptime.c");
    init_keyboard();

    // Initialize interrupts
    init_interrupts();
    __asm__ volatile ("sti"); // Enable interrupts
    log_step("Interrupts enabled.", GREEN);

    draw_floppaos_logo();

    echo("floppaOS - Copyright (C) 2024  Amar Djulovic\n", YELLOW); // copyright notice
    echo("This program is licensed under the GNU General Public License 3.0\nType license for more information\n", CYAN); // license notice

    while (1) {
        scheduler();  // start the scheduler, which runs kernel tasks.
    }
}

// placeholder because C requires a main method but doesn't take the correct types of the magic number and info pointer in eax and ebx
int main() {
    return 0;
}