#include "tmpflopfs.h"
#include "../../apps/echo.h"
#include "../../lib/str.h"
#include "../../drivers/vga/vgahandler.h"
#include "../../drivers/time/floptime.h"
#include "fileutils.h" // Ensure this header has the definition for TmpFileDescriptor
#include <stdint.h>
#include <stdio.h>
#include "fileutils.h"
#include "../../mem/memutils.h"
#define TMP_DISK_SIZE (10 * 1024 * 1024)  // 10 MB simulated disk size
#define TMP_DISK_FILENAME "tmpflopfs.bin"
void tmpflopfs_strcopy(char *dest, const char *src) {
    while ((*dest++ = *src++));
}
// Initialize the Tmpfilesystem
void init_tmpflopfs(struct TmpFileSystem *tmp_fs) {
    tmp_fs->type_id = TMP_FILESYSTEM_TYPE_ID;
    tmp_fs->tmp_file_count = 0;
    tmp_fs->tmp_next_free_offset = 0;
    tmpflopfs_strcopy(tmp_fs->tmp_root_directory, "root");

    // Initialize virtual disk
    TmpFileDescriptor *tmp_disk = flop_open(TMP_DISK_FILENAME, FILE_MODE_WRITE);
    if (tmp_disk == NULL) {
        echo("Failed to initialize virtual disk!\n", RED);
        return;
    }

    // Allocate 1 MB space
    flop_seek(tmp_disk, TMP_DISK_SIZE - 1);
    flop_putc(tmp_disk, 0);
    flop_close(tmp_disk);

    echo("TmpFilesystem initialized!\n", GREEN);
}

// Create a directory (Note: Directories are logical in this system)
void create_tmp_directory(struct TmpFileSystem *tmp_fs, const char *tmp_dirname) {
    if (tmp_fs->tmp_file_count < MAX_TMP_FILES) {
        struct TmpFile *tmp_dir = &tmp_fs->tmp_files[tmp_fs->tmp_file_count++];
        tmpflopfs_strcopy(tmp_dir->name, tmp_dirname);
        tmp_dir->size = 0; // Size 0 for directories
        tmp_dir->data_offset = 0; // No data for directories
        time_get_current(&tmp_dir->created);
        
        echo("Directory created successfully!\n", GREEN);
    } else {
        echo("Max file limit reached. Cannot create directory.\n", RED);
    }
}

// Create a file within the temporary Tmpfilesystem
void create_tmp_file(struct TmpFileSystem *tmp_fs, const char *tmp_filename) {
    if (tmp_fs->tmp_file_count < MAX_TMP_FILES) {
        struct TmpFile *tmp_file = &tmp_fs->tmp_files[tmp_fs->tmp_file_count++];
        tmpflopfs_strcopy(tmp_file->name, tmp_filename);
        tmp_file->size = 0; // File size initially zero
        tmp_file->data_offset = tmp_fs->tmp_next_free_offset;
        time_get_current(&tmp_file->created);

        echo("File created successfully!\n", GREEN);
    } else {
        echo("File limit reached!\n", RED);
    }
}

// Write data to a temporary file
void write_tmp_file(struct TmpFileSystem *tmp_fs, const char *tmp_filename, const char *data, uint32_t size) {
    for (int i = 0; i < tmp_fs->tmp_file_count; i++) {
        struct TmpFile *tmp_file = &tmp_fs->tmp_files[i];
        if (flopstrcmp(tmp_file->name, tmp_filename) == 0) {
            tmp_file->size = size;

            // Write data to virtual disk
            TmpFileDescriptor *disk = flop_open(TMP_DISK_FILENAME, FILE_MODE_WRITE);
            if (disk == NULL) {
                echo("Failed to open virtual disk for writing!\n", RED);
                return;
            }
            flop_seek(disk, tmp_file->data_offset);
            flop_write(disk, data, size);
            flop_close(disk);

            // Update next free offset
            tmp_fs->tmp_next_free_offset += size;

            echo("File data written to disk.\n", GREEN);
            return;
        }
    }
    echo("File not found!\n", RED);
}

// Remove a temporary file
void remove_tmp_file(struct TmpFileSystem *tmp_fs, const char *tmp_filename) {
    for (int i = 0; i < tmp_fs->tmp_file_count; i++) {
        if (flopstrcmp(tmp_fs->tmp_files[i].name, tmp_filename) == 0) {
            // Shift files in the array
            for (int j = i; j < tmp_fs->tmp_file_count - 1; j++) {
                tmp_fs->tmp_files[j] = tmp_fs->tmp_files[j + 1];
            }
            tmp_fs->tmp_file_count--;
            echo("File removed from Tmpfilesystem.\n", GREEN);
            return;
        }
    }
    echo("File not found!\n", RED);
}

// List all files in the temporary Tmpfilesystem
void list_tmp_files(struct TmpFileSystem *tmp_fs, int colored) {
    //char time_buffer[20];
    for (int i = 0; i < tmp_fs->tmp_file_count; i++) {
        const char *tmp_filename = tmp_fs->tmp_files[i].name;
        //time_to_string(&tmp_fs->tmp_files[i].created, time_buffer, sizeof(time_buffer));

        if (colored) {
            unsigned char color = floprand() % 16;
            echo(tmp_filename, color);
        } else {
            echo(tmp_filename, WHITE);
        }
        //echo(" | Created: ", WHITE);
        //echo(time_buffer, WHITE);
        echo("\n", WHITE);
    }
}
void read_tmp_file(struct TmpFileSystem *tmp_fs, const char *tmp_filename) {
    for (int i = 0; i < tmp_fs->tmp_file_count; i++) {
        struct TmpFile *tmp_file = &tmp_fs->tmp_files[i];
        if (flopstrcmp(tmp_file->name, tmp_filename) == 0) {
            // Open the virtual disk
            TmpFileDescriptor *disk = flop_open(TMP_DISK_FILENAME, FILE_MODE_READ);
            if (disk == NULL) {
                echo("Failed to open virtual disk for reading!\n", RED);
                return;
            }

            // Allocate a buffer to read the file data
            char *buffer = (char *)flop_malloc(tmp_file->size + 1);  // +1 for null terminator
            if (buffer == NULL) {
                echo("Failed to allocate memory for file reading!\n", RED);
                flop_close(disk);
                return;
            }

            // Read the file data from the disk
            flop_seek(disk, tmp_file->data_offset);
            size_t bytes_read = flop_read(disk, buffer, tmp_file->size);
            buffer[bytes_read] = '\0';  // Null-terminate the buffer
            flop_close(disk);

            // Print the file contents
            echo("***********File Contents***********\n", CYAN);
            echo(buffer, WHITE);
            echo("\n", WHITE);

            // Free the allocated buffer
            flop_free(buffer);
            return;
        }
    }
    echo("File not found!\n", RED);
}