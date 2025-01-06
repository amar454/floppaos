/*
fshell.c - short for floppaOS shell, fshell is the default shell that floppaOS boots into. At the current moment, it always acts as the "root". It includes functions for file interactions, sleeping, a time display, and exiting.

Copyright 2024 Amar Djulovic <aaamargml@gmail.com>

This file is part of FloppaOS.

FloppaOS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

FloppaOS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with FloppaOS. If not, see <https://www.gnu.org/licenses/>.
*/

#include "fshell.h"
#include "../apps/echo.h"
//#include "../apps/floptxt/floptxt.h"
#include "../fs/flopfs/flopfs.h"
#include "../fs/tmpflopfs/tmpflopfs.h"
#include "../lib/str.h"
#include "../lib/flopmath.h"
#include "../drivers/vga/vgahandler.h"
#include "../drivers/acpi/acpi.h"
#include "../task/task_handler.h"
#include "../drivers/time/floptime.h"
#include "command.h"  // Include the shared command header
#include <stddef.h>
#include <stdint.h>
char current_time_string[32] = "";
#define MAX_COMMAND_LENGTH 128 // Maximum command length
#define MAX_ARGUMENTS 10       // Maximum number of arguments

static void display_prompt() {
    echo("fshell ->  ", WHITE); // Display the shell prompt
}


// Helper function to parse the command
int parse_command(char *command, char *arguments[], int max_arguments) {
    int arg_count = 0;
    char *token = flopstrtok(command, " \n"); // Split by spaces and newlines

    while (token != NULL && arg_count < max_arguments) {
        arguments[arg_count++] = token;
        token = flopstrtok(NULL, " \n");
    }
    return arg_count;
}

// Helper function to handle 'list' command
void handle_list_command(struct FileSystem *fs, struct TmpFileSystem *tmp_fs, char *arguments[], int arg_count) {
    int colored = 0;

    // Check for '--colored' option
    for (int i = 1; i < arg_count; i++) {
        if (flopstrcmp(arguments[i], "--colored") == 0) {
            colored = 1;
        }
    }

    if (fs) {
        list_files(fs, colored);
    } else if (tmp_fs) {
        list_tmp_files(tmp_fs, colored);
    }
}

// Helper function to handle 'license' command
void handle_license_command(int arg_count, char *arguments[]) {
    if (arg_count == 1) {
        // Display full license text
        echo("THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY\n", WHITE);
        echo("APPLICABLE LAW.  EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT\n", WHITE);
        echo("HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY\n", WHITE);
        echo("OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO,\n", WHITE);
        echo("THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR\n", WHITE);
        echo("PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM\n", WHITE);
        echo("IS WITH YOU.  SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF\n", WHITE);
        echo("ALL NECESSARY SERVICING, REPAIR OR CORRECTION.\n", WHITE);
    } else if (arg_count == 2) {
        char *keyword = arguments[1];
        if (flopstrcmp(keyword, "warranty") == 0) {
            echo("THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY\n", WHITE);
        } else if (flopstrcmp(keyword, "purpose") == 0) {
            echo("THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR\n", WHITE);
        } else {
            echo("Keyword not found in the license text. Try 'warranty' or 'purpose'.\n", RED);
        }
    } else {
        echo("Usage: license [optional_keyword]\n", YELLOW);
    }
}

// Helper function to handle 'flopmath' command
void handle_flopmath_command(char *arguments[], int arg_count) {
    if (arg_count < 2) {
        echo("Usage: flopmath <function> [parameters...]\n", YELLOW);
        return;
    }

    char *operation = arguments[1];
    double result = 0.0;
    
    // Dispatch based on the operation
    if (flopstrcmp(operation, "sin") == 0 && arg_count > 2) {
        result = sin(flopatof(arguments[2]));
    } else if (flopstrcmp(operation, "cos") == 0 && arg_count > 2) {
        result = cos(flopatof(arguments[2]));
    } else if (flopstrcmp(operation, "tan") == 0 && arg_count > 2) {
        result = tan(flopatof(arguments[2]));
    } else if (flopstrcmp(operation, "sqrt") == 0 && arg_count > 2) {
        result = sqrt(flopatof(arguments[2]));
    } else if (flopstrcmp(operation, "exp") == 0 && arg_count > 2) {
        result = exp(flopatof(arguments[2]));
    } else if (flopstrcmp(operation, "log") == 0 && arg_count > 2) {
        result = ln(flopatof(arguments[2]));
    } else if (flopstrcmp(operation, "pow") == 0 && arg_count > 3) {
        result = pow(flopatof(arguments[2]), flopatof(arguments[3]));
    } else if (flopstrcmp(operation, "deg_to_rad") == 0 && arg_count > 2) {
        result = deg_to_rad(flopatof(arguments[2]));
    } else if (flopstrcmp(operation, "rad_to_deg") == 0 && arg_count > 2) {
        result = rad_to_deg(flopatof(arguments[2]));
    } else if (flopstrcmp(operation, "abs") == 0 && arg_count > 2) {
        result = fabs(flopatof(arguments[2]));
    } else {
        echo("Unknown or invalid 'flopmath' operation. Available operations:\n", RED);
        echo("sin, cos, tan, sqrt, exp, log, pow, deg_to_rad, rad_to_deg, abs\n", WHITE);
        return;
    }

    // Display the result
    char result_buffer[64];
    flopdtoa(result, result_buffer, sizeof(result_buffer));
    echo("Result: ", GREEN);
    echo(result_buffer, WHITE);
    echo("\n", WHITE);
}


void fshell_task(void *arg) {
    static int initialized = 0; // Track initialization
    char *arguments[MAX_ARGUMENTS];
    char buffer[SECTOR_SIZE]; // Buffer for file reading
    struct FileSystem *fs = NULL;
    struct TmpFileSystem *tmp_fs = NULL;

    // Ensure valid filesystem argument
    if (arg == NULL) {
        echo("Error: No file system provided to fshell.\n", RED);
        return;
    }

    uint8_t type_id = *((uint8_t *)arg); // Determine filesystem type
    if (type_id == FILESYSTEM_TYPE_ID) {
        fs = (struct FileSystem *)arg;
    } else if (type_id == TMP_FILESYSTEM_TYPE_ID) {
        tmp_fs = (struct TmpFileSystem *)arg;
    } else {
        echo("Error: Invalid file system type.\n", RED);
        return;
    }

    // Initialize fshell
    if (!initialized) {
        display_prompt();
        initialized = 1;
        return;
    }

    if (!command_ready) {
        return; // Yield if no command is ready
    }

    // Parse and process the command
    command_ready = 0; // Reset the command-ready flag
    int arg_count = parse_command(command, arguments, MAX_ARGUMENTS);

    if (arg_count == 0) {
        display_prompt();
        return;
    }

    // Command dispatch using switch-case
    char *cmd = arguments[0]; // First argument is the command
    switch (flopstrcmp(cmd, "list") == 0 ? 1 :
        flopstrcmp(cmd, "license") == 0 ? 2 :
        flopstrcmp(cmd, "create") == 0 ? 3 :
        flopstrcmp(cmd, "mkdir") == 0 ? 4 :
        flopstrcmp(cmd, "write") == 0 ? 5 :
        flopstrcmp(cmd, "remove") == 0 ? 6 :
        flopstrcmp(cmd, "read") == 0 ? 7 :
        flopstrcmp(cmd, "help") == 0 ? 8 :
        flopstrcmp(cmd, "exit") == 0 ? 9 :
        flopstrcmp(cmd, "sleep") == 0 ? 10 :
        flopstrcmp(cmd, "tdsp") == 0 ? 11 :
        flopstrcmp(cmd, "flopmath") == 0 ? 12 :
        flopstrcmp(cmd, "shutdown") == 0 ? 13 :
        flopstrcmp(cmd, "vgatest") ==  0 ? 14 : 0)
    {

        case 1: // "list"
            handle_list_command(fs, tmp_fs, arguments, arg_count);
            break;

        case 2: // "license"
            handle_license_command(arg_count, arguments);
            break;

        case 3: // "create"
            if (arg_count > 1) {
                if (fs) create_file(fs, arguments[1]);
                else if (tmp_fs) create_tmp_file(tmp_fs, arguments[1]);
            } else {
                echo("Usage: create <filename>\n", YELLOW);
            }
            break;

        case 4: // "mkdir"
            if (arg_count > 1) {
                if (fs) create_directory(fs, arguments[1]);
                else if (tmp_fs) create_tmp_directory(tmp_fs, arguments[1]);
            } else {
                echo("Usage: mkdir <dirname>\n", YELLOW);
            }
            break;

        case 5: // "write"
            if (arg_count > 2) {
                if (fs) write_file(fs, arguments[1], arguments[2], flopstrlen(arguments[2]));
                else if (tmp_fs) write_tmp_file(tmp_fs, arguments[1], arguments[2], flopstrlen(arguments[2]));
            } else {
                echo("Usage: write <filename> <data>\n", YELLOW);
            }
            break;

        case 6: // "remove"
            if (arg_count > 1) {
                if (fs) remove_file(fs, arguments[1]);
                else if (tmp_fs) remove_tmp_file(tmp_fs, arguments[1]);
            } else {
                echo("Usage: remove <filename>\n", YELLOW);
            }
            break;

        case 7: // "read"
            if (arg_count > 1) {
                if (fs) read_file(fs, arguments[1], buffer, sizeof(buffer));
                else if (tmp_fs) read_tmp_file(tmp_fs, arguments[1]);
            } else {
                echo("Usage: read <filename>\n", YELLOW);
            }
            break;

        case 8: // "help"
            echo("Commands:\n", WHITE);
            echo(" - list [--colored]         List files (with optional color)\n", WHITE);
            echo(" - create <filename>        Create file\n", WHITE);
            echo(" - mkdir <dirname>          Create directory\n", WHITE);
            echo(" - write <filename> <data>  Write data to file\n", WHITE);
            echo(" - remove <filename>        Remove file\n", WHITE);
            echo(" - read <filename>          Read and print file contents\n", WHITE);
            echo(" - sleep <seconds>          Pause execution for specified time\n", WHITE);
            echo(" - license [keyword]        Display license or search by keyword\n", WHITE);
            echo(" - tdsp                     Display info about running tasks\n", WHITE);  // flopmath command prototype
            echo(" - help                     Display this help message\n", WHITE);
            echo(" - vgatest                  Test VGA graphics.", WHITE);
            echo(" - exit                     Exit the shell\n", WHITE);
            break;

        case 9: // "exit"
            echo("Exiting shell...\n", YELLOW);
            initialized = 0; // Reset initialization
            break;
        case 10: // "sleep"
            if (arg_count > 1) {
                sleep_seconds(flopatoi(arguments[1]));
                break;
            } else {
                echo("Usage: sleep <seconds> \n", YELLOW);
            }
            break;
        case 11: // "tdsp"
            print_tasks();
            break;
        case 12: // "flopmath"
            //handle_flopmath_command(arguments, arg_count);
            break;
        case 13: // "shutdown"
            acpi_shutdown();
            break;
        case 14: // "vgatest"
            vga_init();
            vga_clear_screen();
            console_print("Hi, this is a test of my VGA console", YELLOW);
            sleep_seconds(5);
            vga_clear_screen();
            vga_desktop();
            sleep_seconds(10);
            vga_clear_screen();
            break;
        break;
        default: // Unknown command
            echo("Unknown command. Type 'help' for assistance.\n", RED);
            break;
    }

    display_prompt(); // Show prompt for next input
}
