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

// Function declarations
void flopstrcopy(char *dst, const char *src, size_t len);
size_t flopstrlcpy(char *dst, const char *src, size_t size);
int flopatoi(const char *str);
size_t flopstrlen(const char *str);
size_t flopstrnlen(const char *str, size_t maxlen);
int flopstrcmp(const char *s1, const char *s2);
int flopstrncmp(const char *s1, const char *s2, size_t n);
void flopstrrev(char *str);
void flopstrncpy(char *dst, const char *src, size_t n);
void flopstrcat(char *dst, const char *src);
size_t flopstrlcat(char *dst, const char *src, size_t size);
char *flopstrtrim(char *str);
char *flopstrreplace(char *str, const char *old, const char *new_str);
char **flopstrsplit(const char *str, const char *delim);
void flopstrreverse_words(char *str);
char *flopstrstr(const char *haystack, const char *needle);
char *flopstrrchr(const char *str, int c);

// Random number generation functions
unsigned int floprand(void);
void flopsrand(unsigned int seed);

// Simple time function
unsigned int floptime(void);

// Tokenization functions
char *flopstrtok(char *str, const char *delim);
char *flopstrtok_r(char *str, const char *delim, char **saveptr);

// Memory functions
char *flopstrdup(const char *str);

// Character search functions
char *flopstrchr(const char *str, int c);

// Integer to string conversion functions
int flopitoa(int value, char *buffer, int width);
int flopitoa_hex(unsigned int value, char *buffer, int width, int is_upper);

// Formatted output functions
int flopvsnprintf(char *buffer, size_t size, const char *format, va_list args);
int flopsnprintf(char *buffer, size_t size, const char *format, ...);
#endif // STR_H
