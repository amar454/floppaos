#ifndef TIMEUTILS_H
#define TIMEUTILS_H
#include <stddef.h>
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

#endif // TIMEUTILS_H