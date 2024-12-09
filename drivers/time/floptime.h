#ifndef FLOPTIME_H
#define FLOPTIME_H
#include <stddef.h>
void run_timer_for_seconds(int duration, void (*callback)(void));
struct Time {
    unsigned char hour;
    unsigned char minute;
    unsigned char second;
    unsigned char day;
    unsigned char month;
    unsigned int year;
};

// Function declarations
void time_get_current(struct Time *time);
void time_to_string(struct Time *time, char *buffer, size_t size);
void sleep_seconds(int seconds);
void time_task(void *arg);
#endif // FLOPTIME_H