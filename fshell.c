#include "echo.h"
#include "filesystem.h"  
#include "utils/strutils.h"  
#include "vgacolors.h"
#include <stdint.h>

#define MAX_COMMAND_LENGTH 128 // Define max command length
#define MAX_ARGUMENTS 10       // Define max number of arguments

// Function to parse the command using flopstrtok
int parse_command(char *command, char *arguments[], int max_arguments) {
    int arg_count = 0;
    char *token = flopstrtok(command, " \n"); // Split by spaces and newlines

    while (token != NULL && arg_count < max_arguments) {
        arguments[arg_count++] = token;
        token = flopstrtok(NULL, " \n"); // Continue splitting
    }
    return arg_count;
}

void fshell(struct FileSystem *fs) {
    char command[MAX_COMMAND_LENGTH];
    char *arguments[MAX_ARGUMENTS];

    while (1) {
        echo("fshell ->  ", WHITE); // Prompt for command input
        read_input(command, sizeof(command), WHITE); // Read user input

        // Parse the command and get argument count
        int arg_count = parse_command(command, arguments, MAX_ARGUMENTS);

        if (arg_count == 0) {
            continue; // Skip iteration if no command was entered
        }

        // Command processing logic
        if (flopstrcmp(arguments[0], "list") == 0) {
            list_files(fs, 0); // List files without color
        } else if (flopstrcmp(arguments[0], "listc") == 0) {
            list_files(fs, 1); // List files with color
        } else if (flopstrcmp(arguments[0], "create") == 0 && arg_count > 1) {
            create_file(fs, arguments[1]); // Create file
        } else if (flopstrcmp(arguments[0], "mkdir") == 0 && arg_count > 1) {
            create_directory(fs, arguments[1]); // Create directory
        } else if (flopstrcmp(arguments[0], "write") == 0 && arg_count > 2) {
            write_file(fs, arguments[1], arguments[2], flopstrlen(arguments[2])); // Write data to file
        } else if (flopstrcmp(arguments[0], "remove") == 0 && arg_count > 1) {
            remove_file(fs, arguments[1]); // Remove file
        } else if (flopstrcmp(arguments[0], "help") == 0) {
            // Display help information
            echo("Commands:\n", WHITE);
            echo(" - list                  List files without color\n", WHITE);
            echo(" - listc                 List files with color\n", WHITE);
            echo(" - create <filename>     Create file\n", WHITE);
            echo(" - mkdir <dirname>       Create directory\n", WHITE);
            echo(" - write <filename> <data>  Write data to file\n", WHITE);
            echo(" - remove <filename>     Remove file\n", WHITE);
            echo(" - help                  Display this help message\n", WHITE);
            echo(" - exit                  Exit the shell\n", WHITE);
        } else if (flopstrcmp(arguments[0], "exit") == 0) {
            echo("Exiting shell...\n", YELLOW);
            break; // Exit the shell loop
        } else {
            echo("Unknown command. Type 'help' for assistance.\n", RED); // Unknown command message
        }
    }
}