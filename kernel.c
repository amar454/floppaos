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
#include "fs/tmpflopfs/tmpflopfs.h"
#include "fshell/fshell.h"
#include "drivers/keyboard/keyboard.h"
#include "task/task_handler.h"
#include "drivers/vga/vgahandler.h"
#include "mem/memutils.h"
#include "multiboot/multiboot.h"


void clear_screen(void) {
    int index = 0;
    while (index < 80 * 25 * 2) {
        terminal_buffer[index] = ' ';
        index += 2;
    }
}



int main() {
    clear_screen();

    echo("Welcome to floppaOS!\n", YELLOW);
    echo("Type 'help' for available commands.\n\n", WHITE);
    
    //extern multiboot_info_t *multiboot_info;
    //print_multiboot_info(multiboot_info);


    // Initialize memory allocator
    echo("Initializing memory allocator... ", WHITE);
    init_memory();
    echo("Success! \n\n", GREEN);

    // Display loading message for file system
    echo("Loading tmpflopfs File System... ", WHITE);
    struct TmpFileSystem tmp_fs;
    init_tmpflopfs(&tmp_fs);  // Load the filesystem
    echo("Success! \n\n", GREEN);

    // Initialize task system
    echo("Initializing task_handler... ", WHITE);
    initialize_task_system();
    echo("Success! \n\n", GREEN);
    
    // Add fshell and keyboard as tasks
    echo("Adding task fshell... ", WHITE);
    add_task(fshell_task, &tmp_fs, 1);  
    echo("Success! \n\n", GREEN);

    
    echo("Adding task keyboard... ", WHITE);
    add_task(keyboard_task, NULL, 0); 
    echo("Success! \n\n", GREEN);

    // Start the scheduler loop
    while (1) {
        scheduler();  // Execute the next task in the task queue
    }
}

