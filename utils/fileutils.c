#include "fileutils.h"
#include <stdint.h>
#include "strutils.h"
#include "memutils.h"
#include <stdlib.h>
static uint8_t simulated_disk[SIMULATED_DISK_SIZE];  // Simulated disk in memory
static uint32_t next_free_offset = 0;                // Next free position in the "disk"


// Open a file in the simulated filesystem
FileDescriptor *flop_open(const char *filename, int mode) {
    FileDescriptor *fd = (FileDescriptor *)flop_malloc(sizeof(FileDescriptor));
    if (!fd) return NULL;

    if (mode == FILE_MODE_WRITE) {
        fd->position = next_free_offset;
        fd->size = 0;
        fd->data = &simulated_disk[fd->position];
        fd->mode = FILE_MODE_WRITE;
    } else if (mode == FILE_MODE_READ) {
        fd->position = 0;
        fd->size = flopstrlen((char *)&simulated_disk); // Assuming the file is stored as a null-terminated string
        fd->data = (uint8_t *)&simulated_disk;
        fd->mode = FILE_MODE_READ;
    }
    return fd;
}

// Close the file
int flop_close(FileDescriptor *file) {
    if (!file) return -1;
    flop_free(file);  // Use flop equivalent for free
    return 0;
}

// Move the file position to a new offset
int flop_seek(FileDescriptor *file, uint32_t offset) {
    if (offset > file->size) return -1;  // Invalid offset
    file->position = offset;
    return 0;
}

// Write a single character to the file
int flop_putc(FileDescriptor *file, char c) {
    if (file->mode != FILE_MODE_WRITE || file->position >= SIMULATED_DISK_SIZE) return -1;

    file->data[file->position++] = (uint8_t)c;
    if (file->position > file->size) {
        file->size = file->position;  // Update size if writing beyond current size
    }
    return (int)c;
}

// Write a buffer to the file
size_t flop_write(FileDescriptor *file, const void *buffer, size_t size) {
    if (file->mode != FILE_MODE_WRITE) return 0;

    size_t bytes_to_write = size;
    if (file->position + size > SIMULATED_DISK_SIZE) {
        bytes_to_write = SIMULATED_DISK_SIZE - file->position;  // Adjust to available space
    }

    flop_memcpy(&file->data[file->position], buffer, bytes_to_write); // Use flop equivalent for memcpy
    file->position += bytes_to_write;
    if (file->position > file->size) {
        file->size = file->position;
    }
    return bytes_to_write;
}
