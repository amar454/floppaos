#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <stdint.h>
#include "../../mem/vmm.h"
#include "../../mem/pmm.h"
#include "../../drivers/vga/vgahandler.h"
#include "../../drivers/time/floptime.h"
#include "../../lib/str.h"
#include "../../apps/echo.h"
#include "tmpflopfs.h"
extern uint8_t *simulated_disk;
// Define constants for file modes
#define FILE_MODE_READ  0
#define FILE_MODE_WRITE 1

// Next free position in the simulated disk
extern uint32_t tmp_next_free_offset;

// File descriptor structure
typedef struct TmpFileDescriptor {
    uint8_t *data;      // Pointer to file data
    uint32_t size;      // Size of the file
    uint32_t position;  // Current position in the file
    int mode;           // File open mode (read or write)
} TmpFileDescriptor;

// Function prototypes
TmpFileDescriptor *flop_open(const char *tmp_filename, int mode);
int flop_close(TmpFileDescriptor *tmp_file);
int flop_seek(TmpFileDescriptor *tmp_file, uint32_t offset);
int flop_putc(TmpFileDescriptor *tmp_file, char c);
size_t flop_write(TmpFileDescriptor *tmp_file, const void *buffer, size_t size);
size_t flop_read(TmpFileDescriptor *tmp_file, void *buffer, size_t size);

#endif // FILEUTILS_H
