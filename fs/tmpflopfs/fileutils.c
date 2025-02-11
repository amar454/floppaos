#include "fileutils.h"
#include "../../mem/vmm.h"
#include "../../mem/utils.h"
#include "../../drivers/vga/vgahandler.h"
#include "../../drivers/time/floptime.h"
#include "../../mem/pmm.h"
#include "../../apps/echo.h"
#include "../../lib/str.h"
#include <stdlib.h>
#include "tmpflopfs.h"

extern uint8_t *simulated_disk;
uint32_t tmp_next_free_offset = 0;

TmpFileDescriptor *flop_open(const char *tmp_filename, int mode) {
    TmpFileDescriptor *fd = (TmpFileDescriptor *)vmm_malloc(sizeof(TmpFileDescriptor));
    if (!fd) return NULL;

    if (mode == FILE_MODE_WRITE) {
        // Allocate space in the simulated disk
        fd->position = tmp_next_free_offset;
        fd->size = 0;
        fd->data = &simulated_disk[fd->position];
        fd->mode = FILE_MODE_WRITE;
    } else if (mode == FILE_MODE_READ) {
        // Open the file for reading
        fd->position = 0;
        fd->size = flopstrlen((char *)&simulated_disk); // Assuming null-terminated strings
        fd->data = (uint8_t *)&simulated_disk;
        fd->mode = FILE_MODE_READ;
    }

    return fd;
}

// Close the file and free resources
int flop_close(TmpFileDescriptor *tmp_file) {
    if (!tmp_file) return -1;
    vmm_free(tmp_file, tmp_file->size);  // Free the allocated memory for file descriptor
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
    if (tmp_file->mode != FILE_MODE_WRITE || tmp_file->position >= TMP_DISK_SIZE) return -1;

    tmp_file->data[tmp_file->position++] = (uint8_t)c;
    if (tmp_file->position > tmp_file->size) {
        tmp_file->size = tmp_file->position;  // Update size if writing beyond current size
    }
    return (int)c;
}

// Write data to the file
size_t flop_write(TmpFileDescriptor *tmp_file, const void *buffer, size_t size) {
    if (tmp_file->mode != FILE_MODE_WRITE) {
        echo("flop_write: File is not open in write mode.\n", RED);
        return 0;
    }

    // Ensure the file has allocated memory
    if (!tmp_file->data) {
        tmp_file->data = (uint8_t *)vmm_malloc(TMP_DISK_SIZE);  // Allocate memory via VMM
        if (!tmp_file->data) {
            echo("flop_write: Failed to allocate virtual memory for file.\n", RED);
            return 0;
        }
        tmp_file->position = 0;
        tmp_file->size = 0;
    }

    // Adjust the write size to prevent overflow
    size_t bytes_to_write = size;
    if (tmp_file->position + size > TMP_DISK_SIZE) {
        bytes_to_write = TMP_DISK_SIZE - tmp_file->position;
        echo("flop_write: Adjusted write size to fit allocated memory.\n", YELLOW);
    }

    // Copy data into the file memory
    flop_memcpy(&tmp_file->data[tmp_file->position], buffer, bytes_to_write);
    tmp_file->position += bytes_to_write;

    // Update file size if necessary
    if (tmp_file->position > tmp_file->size) {
        tmp_file->size = tmp_file->position;
    }

    return bytes_to_write;
}

// Read data from the file
size_t flop_read(TmpFileDescriptor *tmp_file, void *buffer, size_t size) {
    if (tmp_file->mode != FILE_MODE_READ) return 0;

    size_t bytes_to_read = size;
    if (tmp_file->position + size > tmp_file->size) {
        bytes_to_read = tmp_file->size - tmp_file->position;  // Adjust for available data
    }

    flop_memcpy(buffer, &tmp_file->data[tmp_file->position], bytes_to_read);
    tmp_file->position += bytes_to_read;
    return bytes_to_read;
}
