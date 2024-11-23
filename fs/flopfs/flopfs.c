#include "flopfs.h"
#include "../../apps/echo.h"
#include "../../mem/memutils.h"
#include "../../lib/str.h"
#include "../../drivers/vga/vgahandler.h"
#include "../../drivers/time/floptime.h"
 // Ensure this header has the definition for FileDescriptor
#include <stdint.h>
#include <stdio.h>
#include "../../drivers/io/io.h"
// Function to detect available hard drives (e.g., using IDE/ATA)
void detect_disks() {
    // Typically, IDE controllers expose drives at specific I/O ports
    for (int i = 0; i < 4; i++) {  // Checking the first 4 potential disks
        outb(0x1F6, 0xA0 | (i << 4));  // Select drive
        outb(0x1F7, 0xE0);  // Command to identify the drive

        int timeout = 10000;
        while ((inb(0x1F7) & 0x80) && --timeout > 0);  // Wait for BSY to clear

        if (timeout > 0 && !(inb(0x1F7) & 0x01)) {  // Check if no errors
            echo("Found disk!", GREEN);
            echo("\n", WHITE);
        }
    }
}
void safe_strcopy(char *dst, const char *src, size_t max_len) {
    if (src == NULL) {
        dst[0] = '\0';  // Prevent null pointer dereference
        return;
    }

    size_t i = 0;
    while (i < max_len - 1 && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';  // Null-terminate the string
}
 // Custom string copy that doesn't null-terminate the destination
void flopstrcopy_no_null(char *dst, const char *src, size_t len) {
    // Ensure the length is correct and the source is large enough
    flop_memcpy(dst, src, len);
}
// Read a sector from disk
int read_sector(uint32_t lba, void *buffer) {
    if (buffer == NULL) {
        echo("Buffer is NULL!\n", RED);
        return -1;  // Ensure valid buffer
    }

    // Select drive and LBA bits
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, 1);  // Sector count (1 sector)
    outb(0x1F3, (uint8_t)(lba & 0xFF));  // LBA low byte
    outb(0x1F4, (uint8_t)(lba >> 8));   // LBA mid byte
    outb(0x1F5, (uint8_t)(lba >> 16));  // LBA high byte
    outb(0x1F7, 0x20);  // Command: read sector

    // Wait for BSY to clear and check for errors
    int timeout = 10000;
    while ((inb(0x1F7) & 0x80) && --timeout > 0);  // Wait for BSY to clear
    if (timeout <= 0) {
        echo("Disk read timeout error!\n", RED);
        return -1;  // Timeout error
    }

    // Check if there was an error
    if (inb(0x1F7) & 0x01) {  // Check ERR bit
        echo("Disk read error detected!\n", RED);
        return -1;  // Disk error
    }

    // Check if disk is ready
    if (!(inb(0x1F7) & 0x40)) {  // Ensure DRDY is set
        echo("Disk not ready for read!\n", RED);
        return -1;  // Disk not ready
    }

    // Read the data from the data register
    for (int i = 0; i < SECTOR_SIZE / 2; i++) {
        ((uint16_t *)buffer)[i] = inw(0x1F0);  // Read 16 bits at a time
    }

    return 0;  // Success
}

// Write a sector to disk (LBA mode)
int write_sector(uint32_t lba, const void *buffer) {
    if (buffer == NULL) {
        echo("Buffer is NULL!\n", RED);
        return -1;  // Ensure valid buffer
    }

    // Ensure the I/O permissions are set (depending on your kernel setup)
    // Enable I/O ports here, for example by setting up an I/O permission bitmap.

    // Select drive and LBA bits
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, 1);  // Sector count (1 sector)
    outb(0x1F3, (uint8_t)(lba & 0xFF));  // LBA low byte
    outb(0x1F4, (uint8_t)(lba >> 8));   // LBA mid byte
    outb(0x1F5, (uint8_t)(lba >> 16));  // LBA high byte
    outb(0x1F7, 0x30);  // Command: write sector

    // Wait for BSY to clear and check for errors
    int timeout = 10000;
    while ((inb(0x1F7) & 0x80) && --timeout > 0);  // Wait for BSY to clear
    if (timeout <= 0) {
        echo("Disk write timeout error!\n", RED);
        return -1;  // Timeout error
    }

    // Check if there was an error
    if (inb(0x1F7) & 0x01) {  // Check ERR bit
        echo("Disk write error detected!\n", RED);
        return -1;  // Disk error
    }

    // Check if disk is ready
    if (!(inb(0x1F7) & 0x40)) {  // Ensure DRDY is set
        echo("Disk not ready for write!\n", RED);
        return -1;  // Disk not ready
    }

    // Write the data to the data register
    for (int i = 0; i < SECTOR_SIZE / 2; i++) {
        outw(0x1F0, ((uint16_t *)buffer)[i]);  // Write 16 bits at a time
    }

    // Ensure the write completes successfully
    timeout = 10000;
    while ((inb(0x1F7) & 0x80) && --timeout > 0);  // Wait for BSY to clear
    if (timeout <= 0) {
        echo("Disk write timeout error (after write)!\n", RED);
        return -1;  // Timeout error after writing
    }

    // Check for errors after the write
    if (inb(0x1F7) & 0x01) {
        echo("Disk write error (after write)!\n", RED);
        return -1;  // Check if there was an error
    }

    return 0;  // Success
}


// Check if the disk is writable
int is_disk_writable() {
    char test_sector[SECTOR_SIZE] = {0};
    if (write_sector(0, test_sector) == 0) {
        return 1;  // Disk is writable
    }
    return 0;  // Disk is read-only
}

void init_filesystem(struct FileSystem *fs) {
    fs->type_id = FILESYSTEM_TYPE_ID;
    if (fs == NULL) {
        echo("Filesystem structure is NULL!\n", RED);
        return;
    }

    // Clear the entire filesystem structure to avoid any garbage data.
    flop_memset(fs, 0, sizeof(struct FileSystem));

    // Copy the filesystem signature safely
    flopstrcopy(fs->signature, FILESYSTEM_SIGNATURE, SIGNATURE_LENGTH);
    
    // Initialize the file count and next available offset for file storage
    fs->file_count = 0;
    fs->next_free_offset = SECTOR_SIZE;

    // Initialize the root directory name, ensuring it is within the max length.
    flopstrcopy(fs->root_directory, "root", MAX_PATH_LENGTH);

    // Initialize all file descriptors to zero
    for (int i = 0; i < MAX_FILES; i++) {
        fs->files[i].size = 0;
        fs->files[i].data_offset = 0;
        flopstrcopy(fs->files[i].name, "", sizeof(fs->files[i].name));
    }

    // Additional check to ensure that there is no residual data
    if (fs->file_count > MAX_FILES) {
        echo("File count exceeds maximum limit!\n", RED);
        return;
    }

    // Write the initialized filesystem to disk at sector 1
    if (write_sector(1, fs) != 0) {
        echo("Failed to initialize filesystem on disk!\n", RED);
        return;
    }

    echo("Filesystem initialized successfully.\n", GREEN);
}


// Create a directory in the filesystem
void create_directory(struct FileSystem *fs, const char *dirname) {
    if (fs->file_count < MAX_FILES) {
        struct FileDescriptor *dir = &fs->files[fs->file_count++];
        flopstrcopy(dir->name, dirname, sizeof(dir->name));
        dir->size = 0;  // Size 0 for directories
        dir->data_offset = fs->next_free_offset;  // Directory data offset
        time_get_current(&dir->created);

        // Write updated metadata to the disk
        if (write_sector(1, (void *)fs->files) != 0) {
            echo("Failed to write updated filesystem metadata!\n", RED);
            return;
        }

        echo("Directory created successfully!\n", GREEN);
    } else {
        echo("Max file limit reached. Cannot create directory.\n", RED);
    }
}

// Create a file (check for enough space and limits)
void create_file(struct FileSystem *fs, const char *filename) {
    // Check if the file count has reached the maximum limit
    if (fs->file_count >= MAX_FILES) {
        echo("File limit reached!\n", RED);
        return;
    }

    // Calculate the offset for the new file's metadata
    uint32_t file_offset = fs->next_free_offset;

    // Ensure there is enough space for the file descriptor and its data
    if (file_offset + sizeof(struct FileDescriptor) + SECTOR_SIZE > DISK_SIZE) {
        echo("Not enough space on the disk!\n", RED);
        return;
    }

    // Create a new file structure and initialize its metadata
    struct FileDescriptor *file = &fs->files[fs->file_count++];
    flopstrcopy(file->name, filename, sizeof(file->name));  // Ensure filename fits in the structure
    file->size = 0;  // File size initially zero
    file->data_offset = file_offset;  // Set the file's data offset
    time_get_current(&file->created);  // Set the file's creation time

    // Update the next free offset for future files (file descriptor size is fixed)
    fs->next_free_offset += sizeof(struct FileDescriptor);

    // Write the updated file metadata (only the file descriptor array)
    if (write_sector(1, (void *)fs->files) != 0) {
        echo("Failed to write file metadata to disk!\n", RED);
        return;
    }

    // Prepare empty file data (initialize as zeroes for an empty file)
    char empty_data[SECTOR_SIZE] = {0};
    
    // Write the file data to the disk (at the offset where the file will begin)
    if (write_sector(file->data_offset, empty_data) != 0) {
        echo("Failed to write file data to disk!\n", RED);
        return;
    }

    echo("File created successfully!\n", GREEN);
}



// List files in the filesystem
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
// Read data from an existing file and print its contents
int read_file(struct FileSystem *fs, const char *filename, void *buffer, size_t buffer_size) {
    // Find the file in the filesystem
    struct FileDescriptor *file = NULL;
    for (int i = 0; i < fs->file_count; i++) {
        if (flopstrcmp(fs->files[i].name, filename) == 0) {
            file = &fs->files[i];
            break;
        }
    }

    if (!file) {
        echo("File not found!\n", RED);
        return -1;
    }

    // Check if the buffer is large enough to hold the file's data
    if (buffer_size < file->size) {
        echo("Buffer is too small to hold the file's data!\n", RED);
        return -1;
    }

    // Read the file data from disk into the buffer
    if (read_sector(file->data_offset, buffer) != 0) {
        echo("Failed to read file data from disk!\n", RED);
        return -1;
    }

    // Print the contents of the file
    char *data = (char *)buffer;  // Treat the buffer as a string
    echo("File contents:\n", WHITE);
    for (size_t i = 0; i < file->size; i++) {
        put_char(data[i], WHITE);  // Print each character in the buffer
    }
    echo("\n", WHITE);  // Print a newline after the file contents

    echo("File read and printed successfully!\n", GREEN);
    return 0;
}


// Write data to an existing file
int write_file(struct FileSystem *fs, const char *filename, const void *data, size_t size) {
    // Find the file in the filesystem
    struct FileDescriptor *file = NULL;
    for (int i = 0; i < fs->file_count; i++) {
        if (flopstrcmp(fs->files[i].name, filename) == 0) {
            file = &fs->files[i];
            break;
        }
    }

    if (!file) {
        echo("File not found!\n", RED);
        return -1;
    }

    // Check if the file has enough space to store the data
    uint32_t required_space = file->data_offset + size;
    if (required_space > DISK_SIZE) {
        echo("Not enough space on the disk!\n", RED);
        return -1;
    }

    // If the file is being expanded, update its size
    file->size = size;

    // Write data to the disk at the file's data offset
    if (write_sector(file->data_offset, data) != 0) {
        echo("Failed to write file data to disk!\n", RED);
        return -1;
    }

    // Update the metadata on disk
    if (write_sector(1, (void *)fs->files) != 0) {
        echo("Failed to write updated filesystem metadata!\n", RED);
        return -1;
    }

    echo("File written successfully!\n", GREEN);
    return 0;
}

// Remove a file from the filesystem
int remove_file(struct FileSystem *fs, const char *filename) {
    // Find the file in the filesystem
    int file_index = -1;
    for (int i = 0; i < fs->file_count; i++) {
        if (flopstrcmp(fs->files[i].name, filename) == 0) {
            file_index = i;
            break;
        }
    }

    if (file_index == -1) {
        echo("File not found!\n", RED);
        return -1;
    }

    // Mark the file as deleted by clearing its metadata
    fs->files[file_index].size = 0;
    fs->files[file_index].data_offset = 0;
    flopstrcopy(fs->files[file_index].name, "", sizeof(fs->files[file_index].name));

    // Shift all files after the removed file to fill the gap
    for (int i = file_index; i < fs->file_count - 1; i++) {
        fs->files[i] = fs->files[i + 1];
    }

    // Decrease the file count
    fs->file_count--;

    // Write the updated metadata to disk
    if (write_sector(1, (void *)fs->files) != 0) {
        echo("Failed to write updated filesystem metadata!\n", RED);
        return -1;
    }

    echo("File removed successfully!\n", GREEN);
    return 0;
}
void load_filesystem(struct FileSystem *fs) {
    if (!fs) {
        echo("Error: Null pointer passed to load_filesystem.\n", RED);
        return;
    }

    struct FileSystem temp_fs;

    // Try to read filesystem metadata from disk (sector 1)
    if (read_sector(1, (void *)&temp_fs) != 0) {
        echo("Failed to read filesystem metadata from disk. Initializing a new one...\n", RED);

        // Check if the disk is writable before attempting initialization
        if (!is_disk_writable()) {
            echo("Disk is read-only, cannot initialize filesystem.\n", RED);
            return;
        }

        echo("Initializing a new filesystem...\n", WHITE);
        init_filesystem(fs);  // Initialize the filesystem in memory
    } else {
        // Check if the filesystem signature matches the expected one
        if (flopstrncmp(temp_fs.signature, FILESYSTEM_SIGNATURE, SIGNATURE_LENGTH) == 0) {
            // If signature matches, safely copy the loaded filesystem data
            flop_memcpy(fs, &temp_fs, sizeof(struct FileSystem));
            echo("Filesystem loaded successfully with valid signature.\n", GREEN);
        } else {
            echo("Invalid or corrupted filesystem signature. Initializing a new one...\n", RED);

            // Check if the disk is writable before re-initializing
            if (!is_disk_writable()) {
                echo("Disk is read-only, cannot reinitialize filesystem.\n", RED);
                return;
            }

            echo("Re-initializing the filesystem...\n", WHITE);
            init_filesystem(fs);  // Reinitialize the filesystem in memory
        }
    }
}


void save_filesystem(struct FileSystem *fs) {
    if (!fs) {
        echo("Error: Null pointer passed to save_filesystem.\n", RED);
        return;
    }

    // Copy the filesystem signature without adding a null terminator
    flopstrcopy_no_null(fs->signature, FILESYSTEM_SIGNATURE, SIGNATURE_LENGTH);

    // Debugging: Check if the signature is correctly copied in plain text
    echo("Filesystem signature: ", WHITE);
    for (int i = 0; i < SIGNATURE_LENGTH; i++) {
        echo(&fs->signature[i], YELLOW);
    }
    echo("\n", WHITE);

    // Compare the signature for validity
    if (flop_memcmp(fs->signature, FILESYSTEM_SIGNATURE, SIGNATURE_LENGTH) != 0) {
        echo("Invalid filesystem signature detected. Aborting save operation.\n", RED);
        return;
    }

    // Check if the disk is writable
    if (!is_disk_writable()) {
        echo("Disk is read-only. Changes will not persist.\n", YELLOW);
        return;
    }

    // Attempt to write the filesystem metadata to disk
    if (write_sector(1, (void *)fs) != 0) {
        echo("Failed to save filesystem to disk!\n", RED);
    } else {
        echo("Filesystem saved successfully.\n", GREEN);
    }
}