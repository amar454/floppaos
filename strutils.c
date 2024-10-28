#include "strutils.h"
static char *flopstrtok_next = NULL;
// Copies src string to dest
void flopstrcopy(char *dest, const char *src) {
    while ((*dest++ = *src++));
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
    // Return a pseudo time value (for demonstration purposes)
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