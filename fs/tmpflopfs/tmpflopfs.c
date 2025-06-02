#include "tmpflopfs.h"
#include "../../apps/echo.h"
#include "../../lib/str.h"
#include "../../drivers/vga/vgahandler.h"
#include "../../drivers/time/floptime.h"
#include "fileutils.h"
#include "../../mem/vmm.h"
#include "../../mem/pmm.h"
#include "../../mem/utils.h"
#include <stdint.h>
#include <stdio.h>
#include "../../lib/logging.h"

#include "../../task/sched.h"
#include "../../task/pid.h"
// Structure representing a file in the embedded filesystem

void tmpflopfs_strcopy(char *dest, const char *src) {
    while ((*dest++ = *src++));
}

// Simulated disk memory (global variable)
 uint8_t *simulated_disk = NULL;

void init_tmpflopfs(struct TmpFileSystem *tmp_fs) {
    if (!tmp_fs) {
        log("init_tmpflopfs: Invalid TmpFileSystem pointer!\n", RED);
        return;
    }

    // Initialize file system metadata
    tmp_fs->type_id = TMP_FILESYSTEM_TYPE_ID;
    tmp_fs->tmp_file_count = 0;
    tmp_fs->tmp_next_free_offset = 0;
    flop_memset(tmp_fs->tmp_files, 0, sizeof(tmp_fs->tmp_files));
    flop_memset(tmp_fs->tmp_root_directory, 0, MAX_TMP_PATH_LENGTH);
    tmpflopfs_strcopy(tmp_fs->tmp_root_directory, "root");

    simulated_disk = vmm_malloc(TMP_DISK_SIZE);
    if (!simulated_disk) {
        log("init_tmpflopfs: Failed to allocate memory for simulated disk!\n", RED);
        return;
    }

    // Clear the allocated memory (simulate a blank disk)
    flop_memset(simulated_disk, 0, TMP_DISK_SIZE);

    log("TmpFlopFS initialized successfully with 10 MB virtual disk space!\n", GREEN);
}
void create_tmp_directory(struct TmpFileSystem *tmp_fs, const char *tmp_dirname) {
    if (tmp_fs->tmp_file_count < MAX_TMP_FILES) {
        struct TmpFile *tmp_dir = &tmp_fs->tmp_files[tmp_fs->tmp_file_count++];
        tmpflopfs_strcopy(tmp_dir->name, tmp_dirname);
        tmp_dir->size = 0;
        tmp_dir->data_offset = 0;       
        time_get_current(&tmp_dir->created);

        echo("Directory created successfully!\n", GREEN);
    } else {
        echo("Max file limit reached. Cannot create directory.\n", RED);
    }
}

void create_tmp_file(struct TmpFileSystem *tmp_fs, const char *tmp_filename) {
    if (tmp_fs->tmp_file_count < MAX_TMP_FILES) {
        struct TmpFile *tmp_file = &tmp_fs->tmp_files[tmp_fs->tmp_file_count++];
        tmpflopfs_strcopy(tmp_file->name, tmp_filename);
        tmp_file->size = 0;
        tmp_file->data_offset = tmp_fs->tmp_next_free_offset;
        time_get_current(&tmp_file->created);

        echo("File created successfully!\n", GREEN);
    } else {
        echo("File limit reached!\n", RED);
    }
}

void write_tmp_file(struct TmpFileSystem *tmp_fs, const char *tmp_filename, const char *data, uint32_t size) {
    for (int i = 0; i < tmp_fs->tmp_file_count; i++) {
        struct TmpFile *tmp_file = &tmp_fs->tmp_files[i];
        if (flopstrcmp(tmp_file->name, tmp_filename) == 0) {
            // Check if there's enough space to write
            if (tmp_fs->tmp_next_free_offset + size > TMP_DISK_SIZE) {
                echo("Not enough space on the virtual disk!\n", RED);
                return;
            }

            // Ensure that memory is properly allocated before copying
            uint8_t *dest_addr = simulated_disk + tmp_file->data_offset;
            if (!dest_addr) {
                echo("Failed to access memory for writing data!\n", RED);
                return;
            }

            tmp_file->size = size;
            flop_memcpy(dest_addr, data, size); // Now safe to copy memory

            tmp_fs->tmp_next_free_offset += size;

            echo("File data written successfully!\n", GREEN);
            return;
        }
    }
    echo("File not found!\n", RED);
}


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

void list_tmp_files(struct TmpFileSystem *tmp_fs, int colored) {
    for (int i = 0; i < tmp_fs->tmp_file_count; i++) {
        const char *tmp_filename = tmp_fs->tmp_files[i].name;
        if (colored) {
            unsigned char color = floprand() % 16;
            echo(tmp_filename, color);
        } else {
            echo(tmp_filename, WHITE);
        }
        echo("\n", WHITE);
    }
}

void read_tmp_file(struct TmpFileSystem *tmp_fs, const char *tmp_filename) {
    for (int i = 0; i < tmp_fs->tmp_file_count; i++) {
        struct TmpFile *tmp_file = &tmp_fs->tmp_files[i];
        if (flopstrcmp(tmp_file->name, tmp_filename) == 0) {
            if (tmp_file->size == 0) {
                echo("File is empty!\n", YELLOW);
                return;
            }
            // Read directly from the virtual disk
            char *buffer = (char *)(simulated_disk + tmp_file->data_offset);

            echo("***********File Contents***********\n", CYAN);
            echo(buffer, WHITE);
            echo("\n", WHITE);
            return;
        }
    }
    echo("File not found!\n", RED);
}
