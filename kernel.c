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
#include "fshell/fshell.h"
#include "drivers/keyboard/keyboard.h"
#include "lib/str.h"
#include "task/task_handler.h"
#include "drivers/vga/vgahandler.h"
#include "mem/memutils.h"
#include "multiboot/multiboot.h"
int time_ready = 1; 

void null_task(void *arg)  {
    return;
}
    
int main(int argc, char **argv) {
    echo("Booting floppaOS alpha v0.0.2-alpha...\n", WHITE);
    //test_graphics_mode();
    //while (1) {
    //}
    multiboot_info_t *mbi = (multiboot_info_t *)argv[1];
    echo("[multiboot/multiboot.c]\n", WHITE);
    echo("->Checking for multiboot pointer multiboot_info_t...\n", WHITE);
    sleep_seconds(1);
    // Check for valid Multiboot info structure
    if (mbi && (mbi->flags & MULTIBOOT_INFO_MEMORY)) {
        echo("MULTIBOOT_INFO_MEMORY available.\n\n", GREEN);
        echo_f("MULTIBOOT_INFO_MEMORY address: %p\n", WHITE, mbi);
        // Check if memory information is available
        if (mbi->flags & MULTIBOOT_INFO_MEMORY) {
            //echo("Memory info is available!\n", GREEN);
            //char mem_lower_buffer;
            //flopsnprintf(&mem_lower_buffer, sizeof(mem_lower_buffer), "Lower Memory: %u MB\n", mbi->mem_lower / 1024);
            //echo(&mem_lower_buffer, WHITE);
            //char mem_upper_buffer;
            //flopsnprintf(&mem_upper_buffer, WHITE, "Upper Memory: %u MB\n", mbi->mem_upper / 1024);
            //echo(&mem_upper_buffer, WHITE);

        } else {
            //echo("Memory info is not available!\n", RED);
        }

    } else {
        echo("No valid Multiboot information provided.\n", RED);

    }
    sleep_seconds(1);

    //echo("Initializing ACPI... ", WHITE);
    //acpi_initialize();



    //init_interrupts();


    sleep_seconds(1);
    // Initialize memory allocator
    echo("[mem/memutils.c]\n", WHITE);
    echo("->Initializing memory allocator... ", LIGHT_GRAY);
    init_memory();
    echo("Success! \n", GREEN);


    // Display loading message for file system
    echo("[fs/tmpflopfs/tmpflopfs.c]\n", WHITE);
    echo_f("->Loading tmpflopfs File System... ", LIGHT_GRAY);
    struct TmpFileSystem tmp_fs;
    init_tmpflopfs(&tmp_fs);  // Load the filesystem
    echo("Success! \n", GREEN);



    echo("[task/task_handler.c]\n", WHITE);
    echo("->Initializing task_handler... ", LIGHT_GRAY);
    initialize_task_system();
    echo("Success! \n", GREEN);

    echo("->Adding fshell_task... ", LIGHT_GRAY);
    add_task(fshell_task, &tmp_fs, 0, "fshell", "floppaos://fshell/fshell.c");  
    echo("Success! \n", GREEN);

    echo("->Adding keyboard_task... ", LIGHT_GRAY);
    add_task(keyboard_task, NULL, 1, "keyboard", "floppaos://drivers/keyboard/keyboard.c");
    echo("Success! \n", GREEN);

    echo("->Adding time_task... ", LIGHT_GRAY);
    struct Time system_time; 
    add_task(time_task, &system_time, 2, "floptime", "floppaos://drivers/time/floptime.c");
    echo("Success! \n", GREEN);
    
    const char *ascii_art = 
    "  __ _                          ___  ____   \n"
    " / _| | ___  _ __  _ __   __ _ / _ \\/ ___|  \n"
    "| |_| |/ _ \\| '_ \\| '_ \\ / _` | | | \\___ \\  \n"
    "|  _| | (_) | |_) | |_) | (_| | |_| |___) | \n"
    "|_| |_|\\___/| .__/| .__/ \\__,_|\\___/|____/ v0.1.1-alpha \n"
    "            |_|   |_|                      \n";

    echo(ascii_art, YELLOW);

    echo("floppaOS - Copyright (C) 2024  Amar Djulovic\n", YELLOW);


    echo("This program is licensed under the GNU General Public License 3.0\nType license for more information\n", CYAN);

    echo("Type 'help' for available commands.\n\n", LIGHT_BLUE);

    while (1) {
        scheduler();  // Execute the next task in the task queue
    }
    
}