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

/**
 * @brief Software halt function.
 *
 * @note This function halts the kernel by entering an infinite while loop in C.
 *
 * @warning This function does not halt the CPU, but rather enters an infinite loop in C.
 * @warning This function will leave the system in an unusable state.
 */
void halt() { 
    while (1) {
        continue;
    }
}


void cpuhalt() {
    __asm__ volatile("hlt");
}


/**
 * @brief Kernel panic function.

 * @param address The address that caused the panic.
 * @param msg The error message.
 * @param err The error name.
 * 
 * @note This function is used to panic the kernel when an error occurs.
 * @
 */
void panic(uint32_t address, const char* msg, const char* err) {  
    // Top border
    echo("+", RED);
    for(int i = 0; i < 30; i++) echo("-", RED);
    echo("+\n", RED);

    // Header
    echo("|", RED);
    echo("    KERNEL PANIC ERROR     ", RED);
    echo("|\n", RED);

    // Separator
    echo("|", RED);
    for(int i = 0; i < 30; i++) echo("-", RED);
    echo("|\n", RED);

    // Info lines with padding
    char line[32];
    
    flopsnprintf(line, 31, "| Address: %x", address);
    echo(line, RED);
    for(int i = flopstrnlen(line, 32); i < 31; i++) echo(" ", RED);
    echo("|\n", RED);

    echo("| Message: ", RED);
    echo(msg, LIGHT_RED);
    for(int i = flopstrnlen(msg, 256) + 10; i < 31; i++) echo(" ", RED);
    echo("|\n", RED);

    echo("| Type: ", RED);
    echo(err, LIGHT_RED);
    for(int i = flopstrnlen(err, 256) + 7; i < 31; i++) echo(" ", RED);
    echo("|\n", RED);

    // Separator
    echo("|", RED);
    for(int i = 0; i < 30; i++) echo("-", RED);
    echo("|\n", RED);

    // Contact info
    echo("| Contact: ", YELLOW);
    echo("aaamargml@gmail.com", YELLOW);
    for(int i = 27; i < 31; i++) echo(" ", YELLOW);
    echo("|\n", YELLOW);

    // Bottom border
    echo("+", RED);
    for(int i = 0; i < 30; i++) echo("-", RED);
    echo("+\n", RED);
}/**
 * @brief Memory dump function.
 *
 * @param address The address to start dumping memory from.
 * @param length The number of bytes to dump.

 * @note This function assumes that the memory is 32-bit aligned.

 * @warning This function can easily fill up the screen with a lot of data, so make sure the range is small.
 */
void mem_dump(uint32_t address, uint32_t length) {
    uint32_t *ptr = (uint32_t *)address;
    for (uint32_t i = 0; i < length; i++) {
        if (i % 16 == 0) {
            echo("\n", WHITE);
        }
        log_address("", ptr[i]);
    }
    echo("\n", WHITE);
}
/**
 * @name draw_floppaos_logo
 * @author Amar Djulovic
 * @date 2-23-2025
 *
 * @brief Draws the floppaOS logo in ASCII art.
 */
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
 * @author Amar Djulovic
 * @date 10-24-2024
 *
 * @brief Main kernel entry point
 *
 * @param magic Multiboot magic number
 * @param mb_info Multiboot information structure
 *
 * @return void
 
 * @note This function is responsible for initializing the kernel and setting up the environment for the rest of the kernel to run.
 * @note magic and mb_info should be in eax and ebx respectively.
 *
 * @warning This function should not be called anywhere else in the kernel.
 */
int kmain(uint32_t magic, multiboot_info_t *mb_info) {
    echo("Booting floppaOS alpha v0.0.2-alpha...\n", WHITE);

    // check if multiboot magic and info exist and are valid
    check_multiboot_magic(magic);
    check_multiboot_info(mb_info);
    
    // Print Multiboot information
    print_multiboot_info(mb_info);
    sleep_seconds(1);
    // initialize memory
    init_gdt();
    pmm_init(mb_info);
    slab_init();

    
    sleep_seconds(1); // debugging
    paging_init();
    sleep_seconds(1);
    vmm_init();
    sleep_seconds(1);

    init_interrupts();
    __asm__ volatile("sti");
    // init file system
    struct TmpFileSystem tmp_fs;
    init_tmpflopfs(&tmp_fs);  
    // init task system
    initialize_task_system();
    // init tasks (fshell and keyboard)
    add_task(fshell_task, &tmp_fs, 0, "fshell", "floppaos://fshell/fshell.c");  
    add_task(keyboard_task, NULL, 1, "keyboard", "floppaos://drivers/keyboard/keyboard.c"); // mainly here for fun
    // make time struct and add task
    struct Time system_time; 
    add_task(time_task, &system_time, 2, "floptime", "floppaos://drivers/time/floptime.c"); // cool little time display
    draw_floppaos_logo(); // print cool stuff

    // copyright notice
    echo("floppaOS - Copyright (C) 2024-25 Amar Djulovic <aaamargml@gmail.com>\n", YELLOW); // copyright notice
    echo("This program is licensed under the GNU General Public License 3.0\nType license for more information\n", CYAN); // license notice
    echo("Type help for a list of commands\n", CYAN); // help notice
    
    
    // start scheduler
    while (1) {
        scheduler(); 
    }
}

// bullshit placeholder bc C requires a main method but doesn't take the correct types of the magic number and info pointer in eax and ebx
int main() {
    return 0;
}
