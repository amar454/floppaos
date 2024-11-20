#ifndef FLOPFS_H
#define FLOPFS_H

#include "../../drivers/time/floptime.h"
#include <stdint.h>

// Constants for the filesystem
#define FILESYSTEM_SIGNATURE "FLOPFS"  // Define the filesystem signature
#define SIGNATURE_LENGTH 6 
#define MAX_PATH_LENGTH 100
#define MAX_FILES 100
#define DISK_SIZE 1048576  // Total disk size in bytes (1 MB for example)
#define SECTOR_SIZE 512    // Size of a sector
#define TIMEOUT_LIMIT 10000
#define FILESYSTEM_TYPE_ID 0x01
// Constants for IDE/ATA registers
#define IDE_STATUS_REGISTER  0x1F7
#define IDE_COMMAND_REGISTER 0x1F7
#define IDE_ERROR_REGISTER   0x1F1
#define IDE_SECTOR_COUNT_REG 0x1F2
#define IDE_DATA_REGISTER    0x1F0

#define IDE_READY_BIT 0x40  // DRDY (Drive Ready) bit in status register
#define IDE_ERROR_BIT 0x01  // ERR (Error) bit in status register
#define IDE_BUSY_BIT  0x80  // BSY (Busy) bit in status register
// Structure representing a file descriptor in the embedded filesystem
struct FileDescriptor {
    char name[50];             // File name
    struct Time created;       // Creation time structure
    uint32_t size;             // File size in bytes
    uint32_t data_offset;      // Offset in the "disk" for the file data
};

// Structure representing the file system
struct FileSystem {
    uint8_t type_id;  
    char signature[SIGNATURE_LENGTH];  
    struct FileDescriptor files[MAX_FILES]; // Array of file descriptors
    int file_count;                          // Number of files in the filesystem
    char root_directory[MAX_PATH_LENGTH];    // Root directory path
    uint32_t next_free_offset;               // Next available offset for file data
};

// Function declarations

// Sector I/O operations
int read_sector(uint32_t lba, void *buffer);
int write_sector(uint32_t lba, const void *buffer);

// Filesystem management
void init_filesystem(struct FileSystem *fs);
void create_directory(struct FileSystem *fs, const char *dirname);
void create_file(struct FileSystem *fs, const char *filename);

// File operations
int write_file(struct FileSystem *fs, const char *filename, const void *data, size_t size);
int remove_file(struct FileSystem *fs, const char *filename);
int read_file(struct FileSystem *fs, const char *filename, void *buffer, size_t buffer_size);
// File listing
void list_files(struct FileSystem *fs, int colored);

void load_filesystem(struct FileSystem *fs);
void save_filesystem(struct FileSystem *fs);

// Check if the disk is writable
int is_disk_writable();
#endif // FILESYSTEM_H
