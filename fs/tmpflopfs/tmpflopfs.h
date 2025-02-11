
#ifndef TMPFLOPFS_H
#define TMPFLOPFS_H

#include "../../drivers/time/floptime.h"
#include <stdint.h>
#define TMP_FILESYSTEM_TYPE_ID 0x02
#define MAX_TMP_PATH_LENGTH 100
#define MAX_TMP_FILES 100
#define TMP_DISK_SIZE (1024 * 1024)  
#define TMP_DISK_FILENAME "tmpflopfs.bin"
// Structure representing a file in the embedded filesystem
struct TmpFile {
    char name[50];
    struct Time created;      // Creation time structure
    uint32_t size;            // File size in bytes
    uint32_t data_offset;     // Offset in the "disk" for the file data
};

// Structure representing the file system
struct TmpFileSystem {
    uint8_t type_id;  
    struct TmpFile tmp_files[MAX_TMP_FILES];
    int tmp_file_count;
    char tmp_root_directory[MAX_TMP_PATH_LENGTH]; // Root directory path
    uint32_t tmp_next_free_offset;            // Next available offset for file data
};

// Function declarations
void init_tmpflopfs(struct TmpFileSystem *tmp_fs);
void create_tmp_directory(struct TmpFileSystem *tmp_fs, const char *dirname);
void create_tmp_file(struct TmpFileSystem *tmp_fs, const char *filename);
void write_tmp_file(struct TmpFileSystem *tmp_fs, const char *filename, const char *data, uint32_t size);
void remove_tmp_file(struct TmpFileSystem *tmp_fs, const char *filename);
void list_tmp_files(struct TmpFileSystem *tmp_fs, int colored);
void read_tmp_file(struct TmpFileSystem *tmp_fs, const char *tmp_filename);
#endif // FILESYSTEM_H