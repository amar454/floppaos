/* 

This is the kernel of floppaOS, a free and open-source 32-bit operating system.

The current codename of the operating system is Pine, and the version is 1.0.

Copyright Amar Djulovic 2024

*/

#include "kernel.h"
#include "apps/echo.h"
#include "fs/tmpflopfs/tmpflopfs.h"
#include "fshell/fshell.h"
#include "drivers/keyboard/keyboard.h"
#include "task/task_handler.h"
#include "drivers/vga/vgacolors.h"
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

    // Display loading message
    echo("Loading tmpflopfs File System... ", WHITE);
    // Initialize the FileSystem structure
    struct TmpFileSystem tmp_fs;

    init_tmpflopfs(&tmp_fs);  // Load the filesystem

    // Initialize task system
    echo("Initializing task_handler... ", WHITE);
    initialize_task_system();
    echo("Success! \n", GREEN);
    
    // Add fshell and keyboard as tasks
    echo("Adding task fshell... ", WHITE);
    add_task(fshell_task, &tmp_fs, 1);  
    echo("Success! \n", GREEN);

    echo("Adding task keyboard... ", WHITE);
    add_task(keyboard_task, NULL, 0); 
    echo("Success! \n", GREEN);

    // Start the scheduler loop
    while (1) {
        scheduler();  // Execute the next task in the task queue
    }
}
