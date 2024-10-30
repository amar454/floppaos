#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <stdint.h>
#include <stddef.h>
#define SIMULATED_DISK_SIZE (1024 * 1024)  // 1 MB simulated disk size

// File modes
#define FILE_MODE_READ  0
#define FILE_MODE_WRITE 1

typedef struct {
    uint32_t position;  // Current position in the file
    uint32_t size;      // Size of the file
    uint8_t *data;      // Pointer to the data in the simulated disk
    int mode;           // Mode (read or write)
} FileDescriptor;

// Function prototypes
FileDescriptor *flop_open(const char *filename, int mode);
int flop_close(FileDescriptor *file);
int flop_seek(FileDescriptor *file, uint32_t offset);
int flop_putc(FileDescriptor *file, char c);
size_t flop_write(FileDescriptor *file, const void *buffer, size_t size);
size_t flop_read(FileDescriptor *file, void *buffer, size_t size); // New function prototype

#endif // FILEUTILS_H

