// timeutils.c
#include "timeutils.h"
#include "strutils.h"
#include <stddef.h>
#include "../echo.h"
#include "../vgacolors.h"
// Helper to read a byte from a CMOS register
static unsigned char read_cmos(unsigned char reg) {
    unsigned char val;

    // Write the CMOS register number to 0x70 port
    __asm__ volatile (
        "outb %1, %0"
        : 
        : "dN"((unsigned short)0x70), "a"(reg)
    );
    
    // Read the value from port 0x71
    __asm__ volatile (
        "inb %1, %0"
        : "=a"(val)
        : "dN"((unsigned short)0x71)
    );
    
    return val;
}

// BCD to binary conversion function
static int bcd_to_binary(int val) {
    return ((val / 16) * 10) + (val & 0x0F);
}

// Get current time from CMOS
void time_get_current(struct Time *time) {
    if (time != NULL) {
        // Wait until an update cycle is not in progress
        while (read_cmos(0x0A) & 0x80);

        // Retrieve each time component
        time->second = bcd_to_binary(read_cmos(0x00));
        time->minute = bcd_to_binary(read_cmos(0x02));
        time->hour = bcd_to_binary(read_cmos(0x04));
        time->day = bcd_to_binary(read_cmos(0x07));
        time->month = bcd_to_binary(read_cmos(0x08));
        time->year = bcd_to_binary(read_cmos(0x09)) + 2000;  // Adjust for century if necessary
    }
}

void time_to_string(struct Time *time, char *buffer, size_t size) {
    if (time) {
        // Format the time as "DD-MM-YYYY HH:MM:SS"
        flopsnprintf(buffer, size, "%02d-%02d-%04d %02d:%02d:%02d",
             time->day, time->month, time->year, time->hour, time->minute, time->second);

    } else {
        flopsnprintf(buffer, size, "Invalid time"); // Throw error for weird stuff.
        echo(buffer, RED);
        echo("\n", RED);
    }
}