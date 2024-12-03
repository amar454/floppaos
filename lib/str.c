/*
Copyright 2024 Amar Djulovic <aaamargml@gmail.com>

This file is part of FloppaOS.

FloppaOS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

FloppaOS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with FloppaOS. If not, see <https://www.gnu.org/licenses/>.
*/

#include "str.h"
#include <stdarg.h>
static char *flopstrtok_next = NULL;
// Copies src string to dest
void flopstrcopy(char *dst, const char *src, size_t len) {
    size_t i = 0;
    while (i < len - 1 && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';  // Null-terminate the destination string
}
int flopatoi(const char *str) {
    if (str == NULL) {
        return 0; // Handle null pointer
    }

    int result = 0;
    int sign = 1;

    // Skip leading whitespace
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r' || *str == '\v' || *str == '\f') {
        str++;
    }

    // Check for a sign
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    // Convert characters to integers
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }

    return result * sign;
}
// Returns the length of the string
size_t flopstrlen(const char *str) {
    const char *s = str;
    while (*s) {
        s++;
    }
    return s - str;
}

// Compares two strings
int flopstrcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

// Compares the first n characters of two strings
int flopstrncmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

// Random number generation functions
static unsigned int flop_rand_seed = 1; // Seed for the random number generator

unsigned int floprand(void) {
    flop_rand_seed = flop_rand_seed * 1103515245 + 12345;
    return (flop_rand_seed / 65536) % 32768; // Return a pseudo-random number
}

void flopsrand(unsigned int seed) {
    flop_rand_seed = seed;
}

// Simple time function
unsigned int floptime(void) {
    // Return a pseudo time value 
    // You can modify this to return the time since system start or a fixed epoch if needed.
    static unsigned int seconds_since_start = 0;
    seconds_since_start++; // Increment every time floptime() is called
    return seconds_since_start;
}

char *flopstrtok(char *str, const char *delim) {
    if (str != NULL) {
        flopstrtok_next = str; // Set the current position
    }

    if (flopstrtok_next == NULL) {
        return NULL; // end of tokens 
    }

    // skip leading delims
    while (*flopstrtok_next && flopstrchr(delim, *flopstrtok_next)) {
        flopstrtok_next++;
    }

    if (*flopstrtok_next == '\0') {
        flopstrtok_next = NULL; // end of tokens
        return NULL;
    }

    // find end of token
    char *token_start = flopstrtok_next;
    while (*flopstrtok_next && !flopstrchr(delim, *flopstrtok_next)) {
        flopstrtok_next++;
    }

    // null-terminate token
    if (*flopstrtok_next) {
        *flopstrtok_next++ = '\0';
    }

    return token_start; // return token
}

// Function to find the first occurrence of a character in a string
char *flopstrchr(const char *str, int c) {
    while (*str) {
        if (*str == (char)c) {
            return (char *)str; // Return pointer to the first occurrence
        }
        str++;
    }
    return NULL; // Return NULL if the character is not found
}

// Helper function to get the length of an integer in a specific base (decimal here).
static int flopintlen(int value) {
    int length = 0;
    if (value <= 0) length++; // Space for negative sign or '0'
    while (value != 0) {
        value /= 10;
        length++;
    }
    return length;
}

// Integer to string with zero padding
int flopitoa(int value, char *buffer, int width) {
    char temp[12];
    int len = 0;
    int is_negative = (value < 0);

    if (is_negative) {
        value = -value;
        buffer[len++] = '-';
        width--;  // Adjust for negative sign
    }

    int i = 0;
    do {
        temp[i++] = '0' + (value % 10);
        value /= 10;
    } while (value);

    while (i < width) {
        temp[i++] = '0';
    }

    while (i > 0) {
        buffer[len++] = temp[--i];
    }

    buffer[len] = '\0';
    return len;
}

// Basic vsnprintf for kernel environment with formatting support
int flopvsnprintf(char *buffer, size_t size, const char *format, va_list args) {
    size_t pos = 0;
    for (const char *ptr = format; *ptr && pos < size - 1; ptr++) {
        if (*ptr == '%' && *(ptr + 1)) {
            ptr++;
            int width = 0;

            while (*ptr >= '0' && *ptr <= '9') {
                width = width * 10 + (*ptr - '0');
                ptr++;
            }

            if (*ptr == 'd') { // Integer format
                int num = va_arg(args, int);
                pos += flopitoa(num, buffer + pos, width);
            } else if (*ptr == 's') { // String format
                char *str = va_arg(args, char*);
                while (*str && pos < size - 1) {
                    buffer[pos++] = *str++;
                }
            }
        } else {
            buffer[pos++] = *ptr;
        }
    }
    buffer[pos] = '\0';
    return pos;
}

// snprintf using our custom vsnprintf
int flopsnprintf(char *buffer, size_t size, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int result = flopvsnprintf(buffer, size, format, args);
    va_end(args);
    return result;
}
