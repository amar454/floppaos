/*
Copyright 2024 Amar Djulovic <aaamargml@gmail.com>

This file is part of FloppaOS.

FloppaOS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

FloppaOS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with FloppaOS. If not, see <https://www.gnu.org/licenses/>.
*/

#include "fshell.h"
#include "../apps/echo.h"
#include "../fs/flopfs/flopfs.h"
#include "../fs/tmpflopfs/tmpflopfs.h"
#include "../lib/str.h"
#include "../drivers/vga/vgahandler.h"
#include "../drivers/time/floptime.h"
#include "../mem/memutils.h"
#include "command.h"  // Include the shared command header
#include <stddef.h>
#include <stdint.h>
#define MAX_COMMAND_LENGTH 128 // Define max command length
#define MAX_ARGUMENTS 10       // Define max number of arguments

// Function to parse the command using flopstrtok
int parse_command(char *command, char *arguments[], int max_arguments) {
    int arg_count = 0;
    char *token = flopstrtok(command, " \n"); // Split by spaces and newlines
    int colored = 0;  // Default to no color for listing files
    while (token != NULL && arg_count < max_arguments) {
        arguments[arg_count++] = token;
        token = flopstrtok(NULL, " \n"); // Continue splitting
    }
    return arg_count;
}

void fshell_task(void *arg) {
    static int initialized = 0;  // Track whether fshell_task is initialized
    int colored = 0;
    char *arguments[MAX_ARGUMENTS];
    char buffer[SECTOR_SIZE];  // Buffer to hold file data for reading
    char time_buffer[32];      // Buffer to store the formatted time string
    struct Time current_time;  // Struct to hold the current time

    // Zero-initialize structs to avoid undefined behavior
    flop_memset(&current_time, 0, sizeof(current_time));

    // Check the type of the argument
    struct FileSystem *fs = NULL;
    struct TmpFileSystem *tmp_fs = NULL;

    if (arg == NULL) {
        echo("Error: No file system provided to fshell.\n", RED);
        return;
    }

    uint8_t type_id = *((uint8_t *)arg);  // First byte is type_id in both structs

    if (type_id == FILESYSTEM_TYPE_ID) {
        fs = (struct FileSystem *)arg;
    } else if (type_id == TMP_FILESYSTEM_TYPE_ID) {
        tmp_fs = (struct TmpFileSystem *)arg;
    } else {
        echo("Error: Invalid file system type.\n", RED);
        return;
    }

    // Initialize only once
    if (!initialized) {
        
        echo("fshell ->  ", WHITE);  // Initial prompt
        initialized = 1;  // Mark as initialized
        return;  // Yield back to scheduler
    }

    if (!command_ready) {
        return;  // If no command is ready, yield to scheduler
    }

    // Process the command
    command_ready = 0;  // Clear the command-ready flag
    int arg_count = parse_command(command, arguments, MAX_ARGUMENTS);

    if (arg_count == 0) {
        
        echo("fshell ->  ", WHITE);
        return;  // Yield to scheduler
    }

    // Command processing logic
    if (flopstrcmp(arguments[0], "list") == 0) {
        // Check if '--colored' option is present
        for (int i = 1; i < arg_count; i++) {
            if (flopstrcmp(arguments[i], "--colored") == 0) {
                colored = 1;  // Enable color if --colored is present
            }
        }
        if (fs) {
            list_files(fs, colored);
        } else if (tmp_fs) {
            list_tmp_files(tmp_fs, colored);
        }
    } else if (flopstrcmp(arguments[0], "create") == 0 && arg_count > 1) {
        if (fs) {
            create_file(fs, arguments[1]);
        } else if (tmp_fs) {
            create_tmp_file(tmp_fs, arguments[1]);
        }
    } else if (flopstrcmp(arguments[0], "mkdir") == 0 && arg_count > 1) {
        if (fs) {
            create_directory(fs, arguments[1]);
        } else if (tmp_fs) {
            create_tmp_directory(tmp_fs, arguments[1]);
        }
    } else if (flopstrcmp(arguments[0], "write") == 0 && arg_count > 2) {
        if (fs) {
            write_file(fs, arguments[1], arguments[2], flopstrlen(arguments[2]));
        } else if (tmp_fs) {
            write_tmp_file(tmp_fs, arguments[1], arguments[2], flopstrlen(arguments[2]));
        }
    } else if (flopstrcmp(arguments[0], "remove") == 0 && arg_count > 1) {
        if (fs) {
            remove_file(fs, arguments[1]);
        } else if (tmp_fs) {
            remove_tmp_file(tmp_fs, arguments[1]);
        }
    } else if (flopstrcmp(arguments[0], "sleep") == 0 && arg_count > 1) {
        // Convert the argument to an integer
        int sleep_duration = flopatoi(arguments[1]);
        if (sleep_duration > 0) {
            sleep_seconds(sleep_duration);
        } else {
            echo("Invalid duration. Usage: sleep <seconds>\n", RED);
        }
    } else if (flopstrcmp(arguments[0], "read") == 0 && arg_count > 1) {
        if (fs) {
            read_file(fs, arguments[1], buffer, sizeof(buffer));
        } else if (tmp_fs) {
            read_tmp_file(tmp_fs, arguments[1]);
        }
    } else if (flopstrcmp(arguments[0], "help") == 0) {
        echo("Commands:\n", WHITE);
        echo(" - list [--colored]      List files (with optional color)\n", WHITE);
        echo(" - create <filename>     Create file\n", WHITE);
        echo(" - mkdir <dirname>       Create directory\n", WHITE);
        echo(" - write <filename> <data>  Write data to file\n", WHITE);
        echo(" - remove <filename>     Remove file\n", WHITE);
        echo(" - read <filename>       Read and print file contents\n", WHITE);
        echo(" - sleep <seconds>       Pause execution for specified time\n", WHITE);
        echo(" - help                  Display this help message\n", WHITE);
        echo(" - exit                  Exit the shell\n", WHITE);
    } else if (flopstrcmp(arguments[0], "exit") == 0) {
        echo("Exiting shell...\n", YELLOW);
        initialized = 0;  // Allow reinitialization on re-entry
        return;
    } else {
        echo("Unknown command. Type 'help' for assistance.\n", RED);
    }
    
    echo("fshell ->  ", WHITE);
    return;  // Yield to scheduler
}
