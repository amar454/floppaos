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
#include "mem/gdt.h"
#include "task/task_handler.h"
#include "drivers/vga/vgahandler.h"
#include "mem/paging.h"
#include "lib/logging.h"
#include "multiboot/multiboot.h"
#include "drivers/vga/framebuffer.h"
int time_ready = 1; 

// rudimentary halt function for debugging.
void halt() { 
    while (1) {
        continue;
    }
}

// panic function, with an address, message, and error type.
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

// loaded by the boot.asm
int kmain(uint32_t magic, multiboot_info_t *mb_info) {
    echo("Booting floppaOS alpha v0.0.2-alpha...\n", WHITE);

    // check for Multiboot magic number
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        panic((uint32_t)&magic, "Invalid Multiboot magic number!", "System halted.");
        halt();
    }
    log_step("Valid Multiboot magic number detected.", GREEN);
    // Ensure info pointer is valid
    if (!mb_info) {
        panic(0, "Multiboot info pointer is NULL!", "System halted.");
        halt();
    }
    // Print Multiboot information
    print_multiboot_info(mb_info);
    sleep_seconds(3);

    // initialize memory
    init_gdt();
    pmm_init(mb_info);
    sleep_seconds(3);
    paging_init();
    sleep_seconds(1);
    vmm_init();

    sleep_seconds(1);

    echo("[fs/tmpflopfs/tmpflopfs.c]\n", WHITE);
    echo(" Loading tmpflopfs File System... ", LIGHT_GRAY);
    struct TmpFileSystem tmp_fs;
    init_tmpflopfs(&tmp_fs);  
    echo("Success! \n", GREEN);
    //halt();
    sleep_seconds(1);
    echo("[task/task_handler.c]\n", WHITE);
    echo(" Initializing task_handler... ", LIGHT_GRAY);
    initialize_task_system();
    echo("Success! \n", GREEN);
    

    echo(" Adding fshell_task... ", LIGHT_GRAY);
    add_task(fshell_task, &tmp_fs, 0, "fshell", "floppaos://fshell/fshell.c");  
    echo("Success! \n", GREEN);

    echo(" Adding keyboard_task... ", LIGHT_GRAY);
    add_task(keyboard_task, NULL, 1, "keyboard", "floppaos://drivers/keyboard/keyboard.c");
    echo("Success! \n", GREEN);

    echo(" Adding time_task... ", LIGHT_GRAY);
    struct Time system_time; 
    add_task(time_task, &system_time, 2, "floptime", "floppaos://drivers/time/floptime.c");
    echo("Success! \n", GREEN);

    sleep_seconds(1);
    const char *ascii_art = 
    "  __ _                          ___  ____   \n"
    " / _| | ___  _ __  _ __   __ _ / _ \\/ ___|  \n"
    "| |_| |/ _ \\| '_ \\| '_ \\ / _` | | | \\___ \\  \n"
    "|  _| | (_) | |_) | |_) | (_| | |_| |___) | \n"
    "|_| |_|\\___/| .__/| .__/ \\__,_|\\___/|____/ v0.1.1-alpha \n"
    "            |_|   |_|                      \n";
    sleep_seconds(1);
    echo(ascii_art, YELLOW); // print cool stuff

    echo("floppaOS - Copyright (C) 2024  Amar Djulovic\n", YELLOW); // copyright notice
    echo("This program is licensed under the GNU General Public License 3.0\nType license for more information\n", CYAN); // license notice

    while (1) {
        scheduler();  // start the scheduler, which runs kernel tasks.
    }
}

// bullshit placeholder bc C requires a main method but doesn't take the correct types of the magic number and info pointer in eax and ebx
int main() {
    return 0;
}
