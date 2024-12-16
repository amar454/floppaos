/*
str.c - string functions for floppaOS

Copyright 2024 Amar Djulovic <aaamargml@gmail.com>

This file is part of FloppaOS.

FloppaOS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

FloppaOS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with FloppaOS. If not, see <https://www.gnu.org/licenses/>.
*/

#include "str.h"
#include <stdarg.h>
#include <stdint.h>
#include "../mem/memutils.h"
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

size_t flopstrlcpy(char *dst, const char *src, size_t size) {
    size_t i = 0;
    while (i < size - 1 && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    if (size > 0) {
        dst[i] = '\0';
    }
    return flopstrlen(src);
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

size_t flopstrnlen(const char *str, size_t maxlen) {
    const char *s = str;
    while (*s && maxlen--) {
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

// Reverse a string in place
void flopstrrev(char *str) {
    size_t len = flopstrlen(str);
    size_t i = 0;
    while (i < len / 2) {
        char temp = str[i];
        str[i] = str[len - 1 - i];
        str[len - 1 - i] = temp;
        i++;
    }
}

// Copies up to n characters from src to dst
void flopstrncpy(char *dst, const char *src, size_t n) {
    size_t i = 0;
    while (i < n && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    while (i < n) {
        dst[i] = '\0';
        i++;
    }
}

// Concatenates two strings
void flopstrcat(char *dst, const char *src) {
    while (*dst) {
        dst++;
    }
    while (*src) {
        *dst = *src;
        dst++;
        src++;
    }
    *dst = '\0'; // Null-terminate the resulting string
}

size_t flopstrlcat(char *dst, const char *src, size_t size) {
    size_t dst_len = flopstrlen(dst);
    size_t i = 0;

    if (dst_len < size - 1) {
        while (i < size - dst_len - 1 && src[i] != '\0') {
            dst[dst_len + i] = src[i];
            i++;
        }
        dst[dst_len + i] = '\0';
    }

    return dst_len + flopstrlen(src);
}

char *flopstrtrim(char *str) {
    // Trim leading whitespace
    char *start = str;
    while (*start == ' ' || *start == '\t' || *start == '\n') {
        start++;
    }

    // Trim trailing whitespace
    char *end = start + flopstrlen(start) - 1;
    while (end > start && (*end == ' ' || *end == '\t' || *end == '\n')) {
        end--;
    }

    // Null-terminate the trimmed string
    *(end + 1) = '\0';
    return start;
}

char *flopstrreplace(char *str, const char *old, const char *new_str) {
    char *result = str;
    size_t old_len = flopstrlen(old);
    size_t new_len = flopstrlen(new_str);
    char *temp = (char *)flop_malloc(flopstrlen(str) + 1);
    if (temp) {
        size_t pos = 0;
        while (*str) {
            if (flopstrncmp(str, old, old_len) == 0) {
                flopstrcopy(temp + pos, new_str, new_len);
                pos += new_len;
                str += old_len;
            } else {
                temp[pos++] = *str++;
            }
        }
        temp[pos] = '\0';
        flopstrcopy(result, temp, pos + 1);
        flop_free(temp);
    }
    return result;
}

char **flopstrsplit(const char *str, const char *delim) {
    size_t token_count = 0;
    const char *s = str;
    while (*s) {
        if (flopstrchr(delim, *s)) {
            token_count++;
        }
        s++;
    }

    char **tokens = (char **)flop_malloc((token_count + 2) * sizeof(char *));
    if (tokens) {
        size_t index = 0;
        s = str;
        while (*s) {
            const char *start = s;
            while (*s && !flopstrchr(delim, *s)) {
                s++;
            }
            size_t len = s - start;
            tokens[index] = (char *)flop_malloc(len + 1);
            flopstrcopy(tokens[index], start, len + 1);
            index++;
            if (*s) {
                s++;
            }
        }
        tokens[index] = NULL;
    }

    return tokens;
}

void flopstrreverse_words(char *str) {
    flopstrrev(str);
    char *word_start = str;
    char *p = str;
    while (*p) {
        if (*p == ' ' || *(p + 1) == '\0') {
            if (*p != '\0') {
                *p = '\0';
            }
            flopstrrev(word_start);
            word_start = p + 1;
        }
        p++;
    }
}

// Finds the first occurrence of a substring in a string
char *flopstrstr(const char *haystack, const char *needle) {
    if (*needle == '\0') {
        return (char *)haystack; // Empty string is always found
    }

    for (const char *h = haystack; *h != '\0'; h++) {
        const char *h_tmp = h;
        const char *n_tmp = needle;

        while (*h_tmp && *n_tmp && *h_tmp == *n_tmp) {
            h_tmp++;
            n_tmp++;
        }

        if (*n_tmp == '\0') {
            return (char *)h; // Match found
        }
    }

    return NULL; // No match found
}

// Finds the last occurrence of a character in a string
char *flopstrrchr(const char *str, int c) {
    char *result = NULL;
    while (*str) {
        if (*str == (char)c) {
            result = (char *)str; // Update result to last occurrence
        }
        str++;
    }
    return result; // Return the last occurrence, or NULL if not found
}

// Random number generation functions
static unsigned int flop_rand_seed = 1;

unsigned int floprand(void) {
    flop_rand_seed = flop_rand_seed * 1103515245 + 12345;
    return (flop_rand_seed / 65536) % 32768; // Return a pseudo-random number
}

void flopsrand(unsigned int seed) {
    flop_rand_seed = seed;
}

// Simple time function
unsigned int floptime(void) {
    static unsigned int seconds_since_start = 0;
    seconds_since_start++;
    return seconds_since_start;
}

char *flopstrtok(char *str, const char *delim) {
    if (str != NULL) {
        flopstrtok_next = str; // Set the current position
    }

    if (flopstrtok_next == NULL) {
        return NULL;
    }

    while (*flopstrtok_next && flopstrchr(delim, *flopstrtok_next)) {
        flopstrtok_next++;
    }

    if (*flopstrtok_next == '\0') {
        flopstrtok_next = NULL;
        return NULL;
    }

    char *token_start = flopstrtok_next;
    while (*flopstrtok_next && !flopstrchr(delim, *flopstrtok_next)) {
        flopstrtok_next++;
    }

    if (*flopstrtok_next) {
        *flopstrtok_next++ = '\0';
    }

    return token_start;
}

char *flopstrtok_r(char *str, const char *delim, char **saveptr) {
    if (str == NULL) {
        str = *saveptr;
    }

    while (*str && flopstrchr(delim, *str)) {
        str++;
    }

    if (*str == '\0') {
        *saveptr = NULL;
        return NULL;
    }

    char *token_start = str;
    while (*str && !flopstrchr(delim, *str)) {
        str++;
    }

    if (*str) {
        *str++ = '\0';
    }

    *saveptr = str;
    return token_start;
}

char *flopstrdup(const char *str) {
    size_t len = flopstrlen(str) + 1;
    char *dup = (char *)flop_malloc(len);
    if (dup) {
        flopstrcopy(dup, str, len);
    }
    return dup;
}


char *flopstrchr(const char *str, int c) {
    while (*str) {
        if (*str == (char)c) {
            return (char *)str;
        }
        str++;
    }
    return NULL;
}

static int flopintlen(int value) {
    int length = 0;
    if (value <= 0) length++;
    while (value != 0) {
        value /= 10;
        length++;
    }
    return length;
}

int flopitoa(int value, char *buffer, int width) {
    char temp[12];
    int len = 0;
    int is_negative = (value < 0);

    if (is_negative) {
        value = -value;
        buffer[len++] = '-';
        width--;
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

// Function to handle integer-to-string conversion for base 16 (hex)
int flopitoa_hex(unsigned int value, char *buffer, int width, int is_upper) {
    const char *hex_digits = is_upper ? "0123456789ABCDEF" : "0123456789abcdef";
    char temp[9]; // Maximum size for an unsigned 32-bit hex value
    int len = 0;

    if (value == 0) {
        buffer[len++] = '0';
    }

    int i = 0;
    while (value) {
        temp[i++] = hex_digits[value % 16];
        value /= 16;
    }

    while (i < width) {
        temp[i++] = '0';
    }

    while (i > 0) {
        buffer[len++] = temp[--i];
    }

    buffer[len] = '\0';
    return len;
}

// Implementation of vsnprintf for handling formatted output
int flopvsnprintf(char *buffer, size_t size, const char *format, va_list args) {
    size_t pos = 0;
    for (const char *ptr = format; *ptr && pos < size - 1; ptr++) {
        if (*ptr == '%' && *(ptr + 1)) {
            ptr++; // Skip the '%' character
            int width = 0;

            // Process width specifier (e.g., %5d)
            while (*ptr >= '0' && *ptr <= '9') {
                width = width * 10 + (*ptr - '0');
                ptr++;
            }

            switch (*ptr) {
                case 'd': {  // Handle signed integer format
                    int num = va_arg(args, int);
                    pos += flopitoa(num, buffer + pos, width);
                    break;
                }
                case 'u': {  // Handle unsigned integer format
                    unsigned int num = va_arg(args, unsigned int);
                    pos += flopitoa(num, buffer + pos, width);  // Use itoa for unsigned as well
                    break;
                }
                case 'x': {  // Handle hexadecimal format (lowercase)
                    unsigned int num = va_arg(args, unsigned int);
                    pos += flopitoa_hex(num, buffer + pos, width, 0);  // Lowercase hex
                    break;
                }
                case 'X': {  // Handle uppercase hexadecimal format
                    unsigned int num = va_arg(args, unsigned int);
                    pos += flopitoa_hex(num, buffer + pos, width, 1);  // Uppercase hex
                    break;
                }
                case 's': {  // Handle string format
                    char *str = va_arg(args, char*);
                    while (*str && pos < size - 1) {
                        buffer[pos++] = *str++;
                    }
                    break;
                }
                case 'c': {  // Handle character format (as unsigned char)
                    unsigned char ch = (unsigned char)va_arg(args, int);  // Treat as unsigned char
                    buffer[pos++] = ch;
                    break;
                }
                case 'p': {  // Handle pointer format
                    uintptr_t ptr_value = (uintptr_t)va_arg(args, void*);
                    // Print pointer as hex
                    pos += flopitoa_hex(ptr_value, buffer + pos, width, 0);  // Lowercase hex for pointer
                    break;
                }
                case '%': {  // Handle literal '%' character
                    buffer[pos++] = '%';
                    break;
                }
                default: {  // Handle unrecognized format specifier
                    buffer[pos++] = '%';
                    buffer[pos++] = *ptr;
                    break;
                }
            }
        } else {
            // Handle normal characters
            buffer[pos++] = *ptr;
        }
    }
    buffer[pos] = '\0'; // Null-terminate the string
    return pos;
}
int flopsnprintf(char *buffer, size_t size, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int result = flopvsnprintf(buffer, size, format, args);
    va_end(args);
    return result;
}
