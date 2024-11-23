#include "fshell.h"
#include "../apps/echo.h"
#include "../fs/flopfs/flopfs.h"
#include "../fs/tmpflopfs/tmpflopfs.h"
#include "../lib/str.h"
#include "../drivers/vga/vgahandler.h"
#include "command.h"  // Include the shared command header
#include <stddef.h>
#include <stdint.h>
#define MAX_COMMAND_LENGTH 128 // Define max command length
#define MAX_ARGUMENTS 10       // Define max number of arguments
// Global variables


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
        echo("fshell ->  ", WHITE);  // Display prompt again
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
    } else if (flopstrcmp(arguments[0], "read") == 0 && arg_count > 1) {
        if (fs) {
            read_file(fs, arguments[1], buffer, sizeof(buffer));
        } else if (tmp_fs) {
            //read_tmp_file(tmp_fs, arguments[1], buffer, sizeof(buffer));
        }
    } else if (flopstrcmp(arguments[0], "help") == 0) {
        echo("Commands:\n", WHITE);
        echo(" - list [--colored]      List files (with optional color)\n", WHITE);
        echo(" - create <filename>     Create file\n", WHITE);
        echo(" - mkdir <dirname>       Create directory\n", WHITE);
        echo(" - write <filename> <data>  Write data to file\n", WHITE);
        echo(" - remove <filename>     Remove file\n", WHITE);
        echo(" - read <filename>       Read and print file contents\n", WHITE);
        echo(" - help                  Display this help message\n", WHITE);
    } else if (flopstrcmp(arguments[0], "exit") == 0) {
        echo("Exiting shell...\n", YELLOW);
        initialized = 0;  // Allow reinitialization on re-entry
        return;
    } else {
        echo("Unknown command. Type 'help' for assistance.\n", RED);
    }

    // Display prompt again after processing
    echo("fshell ->  ", WHITE);
    return;  // Yield to scheduler
}
