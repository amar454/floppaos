/* 

Copyright 2024 Amar Djulovic <aaamargml@gmail.com>

This file is part of FloppaOS.

FloppaOS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

FloppaOS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with FloppaOS. If not, see <https://www.gnu.org/licenses/>.

------------------------------------------------------------------------------

kernel.c:

    This is the kernel of floppaOS, a free and open-source 32-bit operating system.
    
    This is the prerelease alpha-v0.0.2 code, still in development

 ------------------------------------------------------------------------------
*/

#include "kernel.h"
#include "apps/echo.h"
#include "drivers/time/floptime.h"
#include "fs/tmpflopfs/tmpflopfs.h"
#include "fshell/command.h"
#include "fshell/fshell.h"
#include "drivers/keyboard/keyboard.h"
#include "mem/lib/str.h"
#include "task/task_handler.h"
#include "drivers/vga/vgahandler.h"
#include "mem/memutils.h"
#include "multiboot/multiboot.h"
int time_ready = 1; 

void clear_screen(void) {
    int index = 0;
    while (index < 80 * 25 * 2) {
        terminal_buffer[index] = ' ';
        index += 2;
    }
}

int main(int argc, char **argv) {
    echo("Booting floppaOS alpha v0.0.2-alpha...\n", WHITE);
    // Assuming argv[1] points to the Multiboot info structure

    multiboot_info_t *mbi = (multiboot_info_t *)argv[1];
    echo("Checking for multiboot pointer multiboot_info_t...\n", WHITE);
    sleep_seconds(1);
    // Check for valid Multiboot info structure
    if (mbi && (mbi->flags & MULTIBOOT_INFO_MEMORY)) {
        echo("MULTIBOOT_INFO_MEMORY available.\n\n", GREEN);

        echo_f("MULTIBOOT_INFO_MEMORY address: %p\n", WHITE, mbi);

        // Check if memory information is available
        if (mbi->flags & MULTIBOOT_INFO_MEMORY) {
            echo("Memory info is available!\n", GREEN);
            echo_f("Lower Memory: %u KB\n", WHITE, mbi->mem_lower);
            echo_f("Upper Memory: %u KB\n", WHITE, mbi->mem_upper);
        } else {
            echo("Memory info is not available!\n", RED);

        }

        // Now print out the actual Multiboot info
        print_multiboot_info(mbi);

    } else {
        echo("No valid Multiboot information provided.\n", RED);

    }


    echo("\nfloppaOS - Copyright (C) 2024  Amar Djulovic\n\n", WHITE);

    sleep_seconds(1);

    echo("This program is licensed under the GNU General Public License 3.0\nType license for more information\n\n", CYAN);

    sleep_seconds(1);
    // Initialize memory allocator
    echo("Initializing memory allocator... ", WHITE);
    init_memory();
    echo("Success! \n\n", GREEN);
    sleep_seconds(1);

    // Display loading message for file system
    echo_f("Loading tmpflopfs File System... ", WHITE);
    struct TmpFileSystem tmp_fs;
    init_tmpflopfs(&tmp_fs);  // Load the filesystem
    echo("Success! \n\n", GREEN);
    sleep_seconds(1);

    // Initialize task system
    echo("Initializing task_handler... ", WHITE);
    initialize_task_system();
    echo("Success! \n\n", GREEN);

    echo("Adding fshell_task... ", WHITE);
    struct Time system_time; 
    add_task(time_task, &system_time, 2);
    echo("Success! \n\n", GREEN);

    // Add fshell and keyboard as tasks
    echo("Adding fshell_task... ", WHITE);
    add_task(fshell_task, &tmp_fs, 1);  
    echo("Success! \n\n", GREEN);

    
    echo("Adding keyboard_task... ", WHITE);
    add_task(keyboard_task, NULL, 0); 
    echo("Success! \n\n", GREEN);
    sleep_seconds(1);
    
    const char *ascii_art = 
    "   ____ __                        ____   ____\n"
    "  / __// /___   ___   ___  ___ _ / __ \\ / __/\n"
    " / _/ / // _ \\ / _ \\ / _ \\/ _ `// /_/ /_\\ \\  \n"
    "/_/  /_/ \\___// .__// .__/\\_,_/ \\____//___/ v0.0.2-alpha\n"
    "             /_/   /_/                                          \n";


    echo(ascii_art, YELLOW);

    echo("\nfloppaOS - Copyright (C) 2024  Amar Djulovic\n\n", WHITE);

    sleep_seconds(1);

    echo("This program is licensed under the GNU General Public License 3.0\nType license for more information\n\n", CYAN);

    echo("Type 'help' for available commands.\n\n", WHITE);

    while (1) {
        scheduler();  // Execute the next task in the task queue
    }
    
}