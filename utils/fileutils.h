#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <stdint.h>
#include <stddef.h>
#include "strutils.h"
#define SIMULATED_DISK_SIZE 1024 * 1024  // 1 MB simulated disk size
#define FILE_MODE_WRITE 1
#define FILE_MODE_READ 0

// File descriptor structure
typedef struct {
    uint32_t position;  // Current position within the file
    uint32_t size;      // File size in bytes
    int mode;           // Read or write mode
    uint8_t *data;      // Pointer to the file's data within the "disk"
} FileDescriptor;

// Function declarations for custom file operations
FileDescriptor *flop_open(const char *filename, int mode);
int flop_close(FileDescriptor *file);
int flop_seek(FileDescriptor *file, uint32_t offset);
int flop_putc(FileDescriptor *file, char c);
size_t flop_write(FileDescriptor *file, const void *buffer, size_t size);
void flop_free(void *ptr);

void *flop_memcpy(void *dest, const void *src, size_t n);
#endif // FILEUTILS_H
