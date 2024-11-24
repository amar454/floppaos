#include "fileutils.h"
#include <stdint.h>
#include "../../mem/memutils.h"
#include <stdlib.h>
#include "../../lib/str.h"
#define SIMULATED_DISK_SIZE 1024 * 1024 // Size of the simulated disk

static uint8_t simulated_disk[SIMULATED_DISK_SIZE];  // Simulated disk in memory
static uint32_t tmp_next_free_offset = 0;           // Next free position in the "disk"

// Open a file in the simulated filesystem
TmpFileDescriptor *flop_open(const char *tmp_filename, int mode) {
    TmpFileDescriptor *fd = (TmpFileDescriptor *)flop_malloc(sizeof(TmpFileDescriptor));
    if (!fd) return NULL;

    if (mode == FILE_MODE_WRITE) {
        fd->position = tmp_next_free_offset;
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
int flop_close(TmpFileDescriptor *tmp_file) {
    if (!tmp_file) return -1;
    flop_free(tmp_file);  // Use flop equivalent for free
    return 0;
}

// Move the file position to a new offset
int flop_seek(TmpFileDescriptor *tmp_file, uint32_t offset) {
    if (offset > tmp_file->size) return -1;  // Invalid offset
    tmp_file->position = offset;
    return 0;
}

// Write a single character to the file
int flop_putc(TmpFileDescriptor *tmp_file, char c) {
    if (tmp_file->mode != FILE_MODE_WRITE || tmp_file->position >= SIMULATED_DISK_SIZE) return -1;

    tmp_file->data[tmp_file->position++] = (uint8_t)c;
    if (tmp_file->position > tmp_file->size) {
        tmp_file->size = tmp_file->position;  // Update size if writing beyond current size
    }
    return (int)c;
}

// Write a buffer to the file
size_t flop_write(TmpFileDescriptor *tmp_file, const void *buffer, size_t size) {
    if (tmp_file->mode != FILE_MODE_WRITE) return 0;

    size_t bytes_to_write = size;
    if (tmp_file->position + size > SIMULATED_DISK_SIZE) {
        bytes_to_write = SIMULATED_DISK_SIZE - tmp_file->position;  // Adjust to available space
    }

    flop_memcpy(&tmp_file->data[tmp_file->position], buffer, bytes_to_write); // Use flop equivalent for memcpy
    tmp_file->position += bytes_to_write;
    if (tmp_file->position > tmp_file->size) {
        tmp_file->size = tmp_file->position;
    }
    return bytes_to_write;
}

// New: Read data from the file
size_t flop_read(TmpFileDescriptor *tmp_file, void *buffer, size_t size) {
    if (tmp_file->mode != FILE_MODE_READ) return 0;

    size_t bytes_to_read = size;
    if (tmp_file->position + size > tmp_file->size) {
        bytes_to_read = tmp_file->size - tmp_file->position;  // Adjust to remaining data
    }

    flop_memcpy(buffer, &tmp_file->data[tmp_file->position], bytes_to_read); // Use flop equivalent for memcpy
    tmp_file->position += bytes_to_read;
    return bytes_to_read;
}