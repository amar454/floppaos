#include "filesystem.h"
#include "echo.h"
#include "strutils.h"
#include "vgacolors.h"

// Define the maximum number of files, adjust as needed
#define MAX_FILES 100  

// Function to create a file
void create_file(struct FileSystem *fs, char *filename) {
    if (fs->file_count < MAX_FILES) {
        // Store the filename in the file system
        flopstrcopy(fs->files[fs->file_count].name, filename);
        fs->files[fs->file_count].name[sizeof(fs->files[fs->file_count].name) - 1] = '\0'; // Ensure null termination
        fs->file_count++;
        echo("File created successfully!\n", GREEN);
    } else {
        echo("File limit reached!\n", RED);
    }
}

// Function to remove a file
void remove_file(struct FileSystem *fs, char *filename) {
    for (int i = 0; i < fs->file_count; i++) {
        if (flopstrcmp(fs->files[i].name, filename) == 0) {
            // Shift files left
            for (int j = i; j < fs->file_count - 1; j++) {
                fs->files[j] = fs->files[j + 1];
            }
            fs->file_count--;
            echo("File removed successfully!\n", GREEN);
            return;
        }
    }
    echo("File not found!\n", RED);
}

// Function to list files with optional coloring
void list_files(struct FileSystem *fs, int colored) {
    // Seed the random number generator
    flopsrand(floptime());

    for (int i = 0; i < fs->file_count; i++) {
        const char *filename = fs->files[i].name;

        if (colored) {
            // Randomly assign a color for each file
            unsigned char color = floprand() % 16; // Random color (0-15)
            echo(filename, color);
        } else {
            echo(filename, WHITE);
        }

        // Print a newline after each filename
        echo("\n", WHITE);
    }
}
