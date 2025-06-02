/*
str.h - string functions for floppaOS

Copyright 2024 Amar Djulovic <aaamargml@gmail.com>

This file is part of FloppaOS.

FloppaOS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

FloppaOS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with FloppaOS. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef STR_H
#define STR_H

#include <stdarg.h>
#include <stddef.h>  // For size_t

#define INT_MIN (-2147483648)
#define INT_MAX 2147483647

void flopdtoa(double value, char *buffer, int precision);
// Copies src string to dest
void flopstrcopy(char *dst, const char *src, size_t len);

// Copies src to dst with limited size and returns src length
size_t flopstrlcpy(char *dst, const char *src, size_t size);

// Converts a string to an integer
int flopatoi(const char *str);

// Returns the length of the string
size_t flopstrlen(const char *str);

// Returns the length of a string with a maximum limit
size_t flopstrnlen(const char *str, size_t maxlen);

// Compares two strings
int flopstrcmp(const char *s1, const char *s2);

// Compares the first n characters of two strings
int flopstrncmp(const char *s1, const char *s2, size_t n);

// Reverses a string in place
void flopstrrev(char *str);

// Copies up to n characters from src to dst
void flopstrncpy(char *dst, const char *src, size_t n);

// Concatenates src to dst
void flopstrcat(char *dst, const char *src);

// Concatenates src to dst with size limit
size_t flopstrlcat(char *dst, const char *src, size_t size);

// Trims leading and trailing whitespace from a string
char *flopstrtrim(char *str);

// Replaces all occurrences of a substring with another string
// Note: This function returns a new string with replacements applied.
char *flopstrreplace(char *str, const char *old, const char *new_str);

// Splits a string into tokens based on delimiters
char **flopstrsplit(const char *str, const char *delim);

// Reverses words in a string
void flopstrreverse_words(char *str);

// Finds the first occurrence of a substring
char *flopstrstr(const char *haystack, const char *needle);

// Finds the last occurrence of a character in a string
char *flopstrrchr(const char *str, int c);

// Generates a pseudo-random number
unsigned int floprand(void);

// Seeds the random number generator
void flopsrand(unsigned int seed);

// Returns a pseudo-timestamp
unsigned int floptime(void);

// Tokenizes a string (non-reentrant)
char *flopstrtok(char *str, const char *delim);

// Tokenizes a string (reentrant)
char *flopstrtok_r(char *str, const char *delim, char **saveptr);

// Duplicates a string
char *flopstrdup(const char *str);

// Finds the first occurrence of a character in a string
char *flopstrchr(const char *str, int c);

// Converts an integer to a string with a minimum width
int flopitoa(int value, char *buffer, int width);

// Converts an integer to a hexadecimal string
int flopitoa_hex(unsigned int value, char *buffer, int width, int is_upper);

// Formats a string using a variable argument list
int flopvsnprintf(char *buffer, size_t size, const char *format, va_list args);

int flopsnprintf(char *buffer, size_t size, const char *format, ...);
// Converts a string to lowercase
void flopstrtolower(char *str);

// Converts a string to uppercase
void flopstrtoupper(char *str);

// Checks if a string is a valid number
int flopstrisnum(const char *str);

// Finds the length of a word in a string based on delimiters
size_t flopstrwordlen(const char *str, const char *delim);

// Skips leading whitespace characters
char *flopstrlskip(char *str);

// Skips trailing whitespace characters
char *flopstrrskip(char *str);

// Concatenates two strings safely with a size limit
int flopstrncat_safe(char *dst, const char *src, size_t size);

// Case-insensitive substring search
char *flopstristr(const char *haystack, const char *needle);

// Extracts a substring from a string
char *flopsubstr(const char *str, size_t start, size_t len);

// Replaces all occurrences of a character in a string
void flopstrreplace_char(char *str, char old_char, char new_char);



double flopatof(const char *str);
#endif // STR_H
    