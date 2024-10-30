#include "filesystem.h"
#include "echo.h"
#include "utils/strutils.h"
#include "vgacolors.h"
#include "utils/timeutils.h"
#include "utils/fileutils.h" // Ensure this header has the definition for FileDescriptor
#include <stdint.h>
#include <stdio.h>

#define DISK_SIZE (1024 * 1024)  // 1 MB simulated disk size
#define DISK_FILENAME "virtual_disk.bin"

// Initialize the filesystem
void init_filesystem(struct FileSystem *fs) {
    fs->file_count = 0;
    fs->next_free_offset = 0;
    flopstrcopy(fs->root_directory, "root");

    // Initialize virtual disk
    FileDescriptor *disk = flop_open(DISK_FILENAME, FILE_MODE_WRITE);
    if (disk == NULL) {
        echo("Failed to initialize virtual disk!\n", RED);
        return;
    }

    // Allocate 1 MB space
    flop_seek(disk, DISK_SIZE - 1);
    flop_putc(disk, 0);
    flop_close(disk);

    echo("Filesystem initialized!\n", GREEN);
}

// Create a directory (Note: Directories are logical in this system)
void create_directory(struct FileSystem *fs, const char *dirname) {
    if (fs->file_count < MAX_FILES) {
        struct File *dir = &fs->files[fs->file_count++];
        flopstrcopy(dir->name, dirname);
        dir->size = 0; // Size 0 for directories
        dir->data_offset = 0; // No data for directories
        time_get_current(&dir->created);
        
        echo("Directory created successfully!\n", GREEN);
    } else {
        echo("Max file limit reached. Cannot create directory.\n", RED);
    }
}

// Create a file within the filesystem
void create_file(struct FileSystem *fs, const char *filename) {
    if (fs->file_count < MAX_FILES) {
        struct File *file = &fs->files[fs->file_count++];
        flopstrcopy(file->name, filename);
        file->size = 0; // File size initially zero
        file->data_offset = fs->next_free_offset;
        time_get_current(&file->created);

        echo("File created successfully!\n", GREEN);
    } else {
        echo("File limit reached!\n", RED);
    }
}

// Write data to a file
void write_file(struct FileSystem *fs, const char *filename, const char *data, uint32_t size) {
    for (int i = 0; i < fs->file_count; i++) {
        struct File *file = &fs->files[i];
        if (flopstrcmp(file->name, filename) == 0) {
            file->size = size;

            // Write data to virtual disk
            FileDescriptor *disk = flop_open(DISK_FILENAME, FILE_MODE_WRITE);
            if (disk == NULL) {
                echo("Failed to open virtual disk for writing!\n", RED);
                return;
            }
            flop_seek(disk, file->data_offset);
            flop_write(disk, data, size);
            flop_close(disk);

            // Update next free offset
            fs->next_free_offset += size;

            echo("File data written to disk.\n", GREEN);
            return;
        }
    }
    echo("File not found!\n", RED);
}

// Read data from a file
void read_file(struct FileSystem *fs, const char *filename, char *buffer, uint32_t size) {
    for (int i = 0; i < fs->file_count; i++) {
        struct File *file = &fs->files[i];
        if (flopstrcmp(file->name, filename) == 0) {
            // Read data from virtual disk
            FileDescriptor *disk = flop_open(DISK_FILENAME, FILE_MODE_READ);
            if (disk == NULL) {
                echo("Failed to open virtual disk for reading!\n", RED);
                return;
            }

            // Ensure the provided buffer is large enough
            if (size < file->size) {
                echo("Provided buffer is too small!\n", RED);
                flop_close(disk);
                return;
            }

            flop_seek(disk, file->data_offset);
            flop_read(disk, buffer, file->size);
            flop_close(disk);

            buffer[file->size] = '\0'; // Null-terminate the buffer

            echo("File contents:\n", GREEN); // Print header
            echo(buffer, WHITE); // Print the file contents
            echo("\n", WHITE);
            return;
        }
    }
    echo("File not found!\n", RED);
}
// Remove a file
void remove_file(struct FileSystem *fs, const char *filename) {
    for (int i = 0; i < fs->file_count; i++) {
        if (flopstrcmp(fs->files[i].name, filename) == 0) {
            // Shift files in the array
            for (int j = i; j < fs->file_count - 1; j++) {
                fs->files[j] = fs->files[j + 1];
            }
            fs->file_count--;
            echo("File removed from filesystem.\n", GREEN);
            return;
        }
    }
    echo("File not found!\n", RED);
}

// List all files in the filesystem
void list_files(struct FileSystem *fs, int colored) {
    char time_buffer[20];
    for (int i = 0; i < fs->file_count; i++) {
        const char *filename = fs->files[i].name;
        time_to_string(&fs->files[i].created, time_buffer, sizeof(time_buffer));

        if (colored) {
            unsigned char color = floprand() % 16;
            echo(filename, color);
        } else {
            echo(filename, WHITE);
        }
        echo(" | Created: ", WHITE);
        echo(time_buffer, WHITE);
        echo("\n", WHITE);
    }
}