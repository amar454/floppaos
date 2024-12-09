// timeutils.c
#include "floptime.h"
#include "../../lib/str.h"
#include <stddef.h>
#include "../../apps/echo.h"
#include "../vga/vgahandler.h"
#include "../../task/task_handler.h"
#include "../../fshell/command.h"
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
        while (read_cmos(0x0A) & 0x80);
        time->second = bcd_to_binary(read_cmos(0x00));
        time->minute = bcd_to_binary(read_cmos(0x02));
        time->hour = bcd_to_binary(read_cmos(0x04));
        time->day = bcd_to_binary(read_cmos(0x07));
        time->month = bcd_to_binary(read_cmos(0x08));
        time->year = bcd_to_binary(read_cmos(0x09)) + 2000;  
    }
}

// Format time into string
void time_to_string(struct Time *time, char *buffer, size_t size) {
    if (time) {
        flopsnprintf(buffer, size, "%02d-%02d-%04d %02d:%02d:%02d",
                     time->day, time->month, time->year,
                     time->hour, time->minute, time->second);
    } else {
        flopsnprintf(buffer, size, "Invalid time");
    }
}
// Standalone timer function
void run_timer_for_seconds(int duration, void (*callback)(void)) {
    struct Time start_time, current_time;
    int elapsed_seconds = 0;

    // Get the start time
    time_get_current(&start_time);

    while (elapsed_seconds < duration) {
        // Get the current time
        time_get_current(&current_time);

        // Calculate elapsed time in seconds
        elapsed_seconds = current_time.second - start_time.second;
        if (elapsed_seconds < 0) {
            // Handle wrapping of seconds (e.g., 58 to 2 seconds)
            elapsed_seconds += 60;
        }

        // Call the provided callback function
        if (callback) {
            callback();
        }
    }
}

// Sleep function
void sleep_seconds(int seconds) {
    struct Time start_time, current_time;
    int elapsed_seconds = 0;

    // Get the start time
    time_get_current(&start_time);

    while (elapsed_seconds < seconds) {
        // Get the current time
        time_get_current(&current_time);

        // Calculate elapsed time
        elapsed_seconds = current_time.second - start_time.second;
        if (elapsed_seconds < 0) {
            // Handle wrap-around (e.g., 58 to 2 seconds)
            elapsed_seconds += 60;
        }
    }
}

void time_task(void *arg) {
    struct Time *time = (struct Time *)arg; // Cast arg to Time struct pointer
    time_get_current(time);
    // Format the time string and store it in the global current_time_string
    flopsnprintf(current_time_string, sizeof(current_time_string), "%02d-%02d-%04d %02d:%02d:%02d",
                 time->day, time->month, time->year,
                 time->hour, time->minute, time->second);
}