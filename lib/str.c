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
#include <stdbool.h>
#include <stdint.h>
#include "../mem/utils.h"
#include "../mem/vmm.h"
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
// Function to convert a double to a string (minimal approach)
void flopdtoa(double value, char *buffer, int precision) {
    // Handle negative values
    if (value < 0) {
        *buffer++ = '-';
        value = -value;
    }

    // Integer part
    uint32_t int_part = (uint32_t)value;
    char *int_str = buffer;
    char tmp[20]; // Temporary buffer for reversing the integer part
    int i = 0;
    do {
        tmp[i++] = (int_part % 10) + '0';
        int_part /= 10;
    } while (int_part > 0);

    // Reverse the integer part
    while (i > 0) {
        *buffer++ = tmp[--i];
    }

    // Fractional part
    if (precision > 0) {
        *buffer++ = '.'; // Decimal point
        value -= (uint32_t)value; // Remove the integer part

        while (precision--) {
            value *= 10;
            uint32_t frac_part = (uint32_t)value;

            // Directly write the fraction part (based on the precision level)
            switch (frac_part) {
                case 0: *buffer++ = '0'; break;
                case 1: *buffer++ = '1'; break;
                case 2: *buffer++ = '2'; break;
                case 3: *buffer++ = '3'; break;
                case 4: *buffer++ = '4'; break;
                case 5: *buffer++ = '5'; break;
                case 6: *buffer++ = '6'; break;
                case 7: *buffer++ = '7'; break;
                case 8: *buffer++ = '8'; break;
                case 9: *buffer++ = '9'; break;
                default: *buffer++ = '0'; break;
            }

            value -= frac_part; // Remove the integer part of the fraction
        }
    }

    *buffer = '\0'; // Null-terminate the string
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
    char *temp = (char *)vmm_malloc(flopstrlen(str) + 1);
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
        vmm_free(temp, flopstrlen(str) + 1);
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

    char **tokens = (char **)vmm_malloc((token_count + 2) * sizeof(char *));
    if (tokens) {
        size_t index = 0;
        s = str;
        while (*s) {
            const char *start = s;
            while (*s && !flopstrchr(delim, *s)) {
                s++;
            }
            size_t len = s - start;
            tokens[index] = (char *)vmm_malloc(len + 1);
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
    char *dup = (char *)vmm_malloc(len);
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
    char temp[12];  // Enough for INT_MIN
    int len = 0;
    int is_negative = (value < 0);

    // Handle INT_MIN correctly
    if (value == INT_MIN) {
        const char *int_min_str = "-2147483648";
        while (*int_min_str) {
            buffer[len++] = *int_min_str++;
        }
        buffer[len] = '\0';
        return len;
    }

    // Handle negative numbers (other than INT_MIN)
    if (is_negative) {
        value = -value;
    }

    int i = 0;
    do {
        temp[i++] = '0' + (value % 10);
        value /= 10;
    } while (value);

    // Apply padding (if necessary)
    int padding = width - i - (is_negative ? 1 : 0);
    while (padding > 0) {
        temp[i++] = '0';  // Add padding zeros
        padding--;
    }

    // Add negative sign if needed
    if (is_negative) {
        buffer[len++] = '-';
    }

    // Copy digits in reverse order
    while (i > 0) {
        buffer[len++] = temp[--i];
    }

    buffer[len] = '\0';  // Null-terminate
    return len;
}



// Function to handle integer-to-string conversion for base 16 (hex)
int flopitoa_hex(unsigned int value, char *buffer, int width, int is_upper) {
    const char *hex_digits = is_upper ? "0123456789ABCDEF" : "0123456789abcdef";
    char temp[9]; // Maximum size for an unsigned 32-bit hex value (8 digits + '\0')
    int len = 0;

    // Handle the zero case explicitly
    if (value == 0) {
        temp[len++] = '0';
    }

    // Convert value to hexadecimal
    int i = 0;
    while (value) {
        temp[i++] = hex_digits[value % 16];
        value /= 16;
    }

    // Apply width padding with zeroes
    while (i < width) {
        temp[i++] = '0';
    }

    // Copy the reversed result into the buffer
    while (i > 0) {
        buffer[len++] = temp[--i];
    }

    buffer[len] = '\0'; // Null-terminate the string
    return len;
}






// Implementation of vsnprintf for handling formatted output
int flopvsnprintf(char *buffer, size_t size, const char *format, va_list args) {
    size_t pos = 0;

    for (const char *ptr = format; *ptr && pos < size - 1; ptr++) {
        if (*ptr == '%' && *(ptr + 1)) {
            ptr++; // Skip the '%' character
            int width = 0;
            int left_align = 0;

            // Check for '-' flag (left alignment)
            if (*ptr == '-') {
                left_align = 1;
                ptr++;
            }

            // Process width specifier (e.g., %5d)
            while (*ptr >= '0' && *ptr <= '9') {
                width = width * 10 + (*ptr - '0');
                ptr++;
            }

            switch (*ptr) {
                case 'd': { // Handle integers
                    int num = va_arg(args, int);
                    char temp[12]; // Buffer for number (enough for INT_MIN)
                    int len = flopitoa(num, temp, 0); // Convert number to string
                    int padding = (width > len) ? width - len : 0;

                    if (!left_align) {
                        // Add padding before number
                        while (padding-- > 0 && pos < size - 1) {
                            buffer[pos++] = ' ';
                        }
                    }

                    // Copy number
                    for (int i = 0; i < len && pos < size - 1; i++) {
                        buffer[pos++] = temp[i];
                    }

                    if (left_align) {
                        // Add padding after number
                        while (padding-- > 0 && pos < size - 1) {
                            buffer[pos++] = ' ';
                        }
                    }
                    break;
                }
                case 'u': { // Handle unsigned integers
                    unsigned int num = va_arg(args, unsigned int);
                    char temp[12];
                    int len = flopitoa(num, temp, 0);
                    int padding = (width > len) ? width - len : 0;

                    if (!left_align) {
                        while (padding-- > 0 && pos < size - 1) {
                            buffer[pos++] = ' ';
                        }
                    }

                    for (int i = 0; i < len && pos < size - 1; i++) {
                        buffer[pos++] = temp[i];
                    }

                    if (left_align) {
                        while (padding-- > 0 && pos < size - 1) {
                            buffer[pos++] = ' ';
                        }
                    }
                    break;
                }
                case 'x': case 'X': { // Handle hexadecimal
                    unsigned int num = va_arg(args, unsigned int);
                    char temp[12];
                    int len = flopitoa_hex(num, temp, width, (*ptr == 'X')); // Uppercase for 'X'
                    int padding = (width > len) ? width - len : 0;

                    if (!left_align) {
                        while (padding-- > 0 && pos < size - 1) {
                            buffer[pos++] = ' ';
                        }
                    }

                    for (int i = 0; i < len && pos < size - 1; i++) {
                        buffer[pos++] = temp[i];
                    }

                    if (left_align) {
                        while (padding-- > 0 && pos < size - 1) {
                            buffer[pos++] = ' ';
                        }
                    }
                    break;
                }
                case 's': { // Handle strings
                    char *str = va_arg(args, char *);
                    size_t len = 0;
                    while (str[len]) len++; // Calculate string length
                    int padding = (width > (int)len) ? width - (int)len : 0;

                    if (!left_align) {
                        while (padding-- > 0 && pos < size - 1) {
                            buffer[pos++] = ' ';
                        }
                    }

                    for (size_t i = 0; i < len && pos < size - 1; i++) {
                        buffer[pos++] = str[i];
                    }

                    if (left_align) {
                        while (padding-- > 0 && pos < size - 1) {
                            buffer[pos++] = ' ';
                        }
                    }
                    break;
                }
                case 'c': { // Handle characters
                    char ch = (char)va_arg(args, int);
                    int padding = (width > 1) ? width - 1 : 0;

                    if (!left_align) {
                        while (padding-- > 0 && pos < size - 1) {
                            buffer[pos++] = ' ';
                        }
                    }

                    buffer[pos++] = ch;

                    if (left_align) {
                        while (padding-- > 0 && pos < size - 1) {
                            buffer[pos++] = ' ';
                        }
                    }
                    break;
                }
                case 'p': { // Handle pointers
                    uintptr_t ptr_value = (uintptr_t)va_arg(args, void *);
                    char temp[20];
                    int len = flopitoa_hex(ptr_value, temp, width, 0);  // Use flopitoa_hex for pointers
                    int padding = (width > len) ? width - len : 0;

                    if (!left_align) {
                        while (padding-- > 0 && pos < size - 1) {
                            buffer[pos++] = ' ';
                        }
                    }

                    for (int i = 0; i < len && pos < size - 1; i++) {
                        buffer[pos++] = temp[i];
                    }

                    if (left_align) {
                        while (padding-- > 0 && pos < size - 1) {
                            buffer[pos++] = ' ';
                        }
                    }
                    break;
                }
                case '%': { // Handle literal '%'
                    buffer[pos++] = '%';
                    break;
                }
                default: { // Unrecognized format
                    buffer[pos++] = '%';
                    buffer[pos++] = *ptr;
                    break;
                }
            }
        } else {
            buffer[pos++] = *ptr; // Regular characters
        }
    }

    buffer[pos] = '\0'; // Null-terminate
    return pos;
}

// Convert a string to lowercase
void flopstrtolower(char *str) {
    while (*str) {
        if (*str >= 'A' && *str <= 'Z') {
            *str = *str + ('a' - 'A');
        }
        str++;
    }
}

// Convert a string to uppercase
void flopstrtoupper(char *str) {
    while (*str) {
        if (*str >= 'a' && *str <= 'z') {
            *str = *str - ('a' - 'A');
        }
        str++;
    }
}

// Check if a string is a valid number
int flopstrisnum(const char *str) {
    if (*str == '-' || *str == '+') {
        str++; // Skip sign
    }

    while (*str) {
        if (*str < '0' || *str > '9') {
            return 0; // Not a number
        }
        str++;
    }
    return 1; // It is a valid number
}

// Find the length of a word in a string (until a delimiter is encountered)
size_t flopstrwordlen(const char *str, const char *delim) {
    size_t len = 0;
    while (*str && !flopstrchr(delim, *str)) {
        str++;
        len++;
    }
    return len;
}

// Find the first non-space character in a string
char *flopstrlskip(char *str) {
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') {
        str++;
    }
    return str;
}

// Find the last non-space character in a string
char *flopstrrskip(char *str) {
    while (*str && (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r')) {
        str--;
    }
    return str;
}

// Safe string concatenation with specified length
int flopstrncat_safe(char *dst, const char *src, size_t size) {
    size_t dst_len = flopstrlen(dst);
    size_t src_len = flopstrlen(src);

    if (dst_len + src_len + 1 > size) {
        return -1; // Not enough space
    }

    flopstrcat(dst, src);
    return 0;
}

// String search (case-insensitive)
char *flopstristr(const char *haystack, const char *needle) {
    char *h = (char *)haystack;
    char *n = (char *)needle;

    // Convert both haystack and needle to lowercase to perform case-insensitive comparison
    char *h_lower = (char *)vmm_malloc(flopstrlen(haystack) + 1);  // Allocate memory for lowercase copy of haystack
    char *n_lower = (char *)vmm_malloc(flopstrlen(needle) + 1);    // Allocate memory for lowercase copy of needle

    if (h_lower && n_lower) {
        flopstrcopy(h_lower, haystack, flopstrlen(  h_lower));  // Copy haystack to lowercase version
        flopstrcopy(n_lower, needle, flopstrlen(  n_lower));    // Copy needle to lowercase version

        flopstrtolower(h_lower);  // Convert haystack to lowercase
        flopstrtolower(n_lower);  // Convert needle to lowercase

        while (*h_lower) {
            char *h_tmp = h_lower;
            char *n_tmp = n_lower;

            while (*h_tmp && *n_tmp && (*h_tmp == *n_tmp)) {
                h_tmp++;
                n_tmp++;
            }

            if (*n_tmp == '\0') {
                vmm_free(h_lower, flopstrlen(  h_lower));  // Free allocated memory for lowercase copies
                vmm_free(n_lower, flopstrlen(  n_lower));
                return h; // Match found
            }
            h_lower++;
        }
    }

    vmm_free(h_lower, flopstrlen(  h_lower));  // Free the lowercase copies
    vmm_free(n_lower, flopstrlen(  n_lower));
    return NULL;  // No match found
}

// Extract a substring from a string
char *flopsubstr(const char *str, size_t start, size_t len) {
    if (start >= flopstrlen(str)) {
        return NULL; // Start is out of bounds
    }

    char *sub = (char *)vmm_malloc(len + 1);
    if (!sub) {
        return NULL; // Memory allocation failed
    }

    size_t i = 0;
    while (i < len && str[start + i] != '\0') {
        sub[i] = str[start + i];
        i++;
    }
    sub[i] = '\0';

    return sub;
}

// Replace all occurrences of a character in a string
void flopstrreplace_char(char *str, char old_char, char new_char) {
    while (*str) {
        if (*str == old_char) {
            *str = new_char;
        }
        str++;
    }
}

// Find the position of a character in a string (case-insensitive)
int flopstrichr(const char *str, char c) {
    // Convert character c to lowercase
    char lower_c = c;
    if (lower_c >= 'A' && lower_c <= 'Z') {
        lower_c = lower_c + ('a' - 'A');
    }

    int pos = 0;

    // Create a temporary copy of the string to avoid modifying the original
    char *temp_str = (char *)vmm_malloc(flopstrlen(str) + 1);
    if (temp_str) {
        // Copy string to temp_str
        flopstrcopy(temp_str, str,flopstrlen(str));
        
        // Convert the copied string to lowercase
        flopstrtolower(temp_str);

        // Search for the lowercase character in the lowercase string
        while (temp_str[pos] != '\0') {
            if (temp_str[pos] == lower_c) {
                vmm_free(temp_str,flopstrlen(temp_str)); // Clean up the temporary string
                return pos;
            }
            pos++;
        }

        vmm_free(temp_str,flopstrlen(temp_str)); // Clean up the temporary string
    }

    return -1; // Character not found
}


// Return a string representing a binary number (e.g., "1101")
char *flopitoa_bin(unsigned int value, char *buffer, int width) {
    char temp[33]; // Maximum size for a 32-bit unsigned integer
    int len = 0;

    if (value == 0) {
        buffer[len++] = '0';
    }

    int i = 0;
    while (value) {
        temp[i++] = (value % 2) + '0';
        value /= 2;
    }

    while (i < width) {
        temp[i++] = '0';
    }

    while (i > 0) {
        buffer[len++] = temp[--i];
    }

    buffer[len] = '\0';
    return buffer;
}
// Helper function to check if a character is a digit
bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

// Helper function to check if a character is a decimal point
bool is_decimal_point(char c) {
    return c == '.';
}

double flopatof(const char *str) {
    double result = 0.0;
    double fraction = 0.0;
    double sign = 1.0;

    // Handle possible sign at the start
    if (*str == '-') {
        sign = -1.0;
        str++;
    } else if (*str == '+') {
        str++;
    }

    // Process the integer part
    while (*str >= '0' && *str <= '9') {
        result = result * 10.0 + (*str - '0');
        str++;
    }

    // Process the fractional part, if any
    if (*str == '.') {
        str++;
        double place_value = 0.1;
        while (*str >= '0' && *str <= '9') {
            result += (*str - '0') * place_value;
            place_value *= 0.1;
            str++;
        }
    }

    // Process the exponent part, if any (e.g., "e" or "E")
    if (*str == 'e' || *str == 'E') {
        str++;
        int exp_sign = 1;
        if (*str == '-') {
            exp_sign = -1;
            str++;
        } else if (*str == '+') {
            str++;
        }

        int exponent = 0;
        while (*str >= '0' && *str <= '9') {
            exponent = exponent * 10 + (*str - '0');
            str++;
        }

        // Apply the exponent
        while (exponent--) {
            if (exp_sign == 1) {
                result *= 10;
            } else {
                result /= 10;
            }
        }
    }

    return result * sign;  // Return the result with the correct sign
}





int flopsnprintf(char *buffer, size_t size, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int result = flopvsnprintf(buffer, size, format, args);
    va_end(args);
    return result;
}
