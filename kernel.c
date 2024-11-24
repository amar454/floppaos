/* 

MIT License

Copyright (c) 2024 Amar Djulovic 

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

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

