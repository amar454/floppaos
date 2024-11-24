#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <stdint.h>
#include <stddef.h>

// File modes
#define FILE_MODE_READ  1
#define FILE_MODE_WRITE 2

// Temporary file descriptor structure
typedef struct TmpFileDescriptor {
    uint32_t position;    // Current position in the file
    uint32_t size;        // Size of the file
    uint8_t *data;        // Pointer to the file data
    int mode;             // Mode (read or write)
} TmpFileDescriptor;

// Function declarations
TmpFileDescriptor *flop_open(const char *tmp_filename, int mode);
int flop_close(TmpFileDescriptor *tmp_file);
int flop_seek(TmpFileDescriptor *tmp_file, uint32_t offset);
int flop_putc(TmpFileDescriptor *tmp_file, char c);
size_t flop_write(TmpFileDescriptor *tmp_file, const void *buffer, size_t size);
size_t flop_read(TmpFileDescriptor *tmp_file, void *buffer, size_t size);

#endif // FILEUTILS_H
