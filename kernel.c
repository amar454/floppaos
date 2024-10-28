/* 

This is the kernel of floppaOS a free and open source 32 bit operating system.

The current codename of the operating system is Pine, and the version 1.0.

Copyright Amar Djulovic 2024

*/

#include "kernel.h"
#include "echo.h"
#include "fshell.h"
#include "filesystem.h"
#include "vgacolors.h"

void clear_screen(void) {
    int index = 0;
    while (index < 80 * 25 * 2) {
        terminal_buffer[index] = ' ';
        index += 2;
    }
}

void main(void) {
    clear_screen(); // Clear the screen
    echo("Welcome to floppaOS!\n", YELLOW); // Display welcome message
    echo("Type 'help' for available commands.\n", WHITE); // Display help message

    struct FileSystem fs; // Create an instance of the FileSystem struct
    fs.file_count = 0; // Initialize file count to 0

    // Start the command line interface (shell)
    fshell(&fs); // Call the fshell function with the file system instance
}
