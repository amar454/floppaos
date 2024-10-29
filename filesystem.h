#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "utils/timeutils.h"
#include <stdint.h>

#define MAX_PATH_LENGTH 100
#define MAX_FILES 100

// Structure representing a file in the embedded filesystem
struct File {
    char name[50];
    struct Time created;      // Creation time structure
    uint32_t size;            // File size in bytes
    uint32_t data_offset;     // Offset in the "disk" for the file data
};

// Structure representing the file system
struct FileSystem {
    struct File files[MAX_FILES];
    int file_count;
    char root_directory[MAX_PATH_LENGTH]; // Root directory path
    uint32_t next_free_offset;            // Next available offset for file data
};

// Function declarations
void init_filesystem(struct FileSystem *fs);
void create_directory(struct FileSystem *fs, const char *dirname);
void create_file(struct FileSystem *fs, const char *filename);
void write_file(struct FileSystem *fs, const char *filename, const char *data, uint32_t size);
void remove_file(struct FileSystem *fs, const char *filename);
void list_files(struct FileSystem *fs, int colored);

#endif // FILESYSTEM_H
