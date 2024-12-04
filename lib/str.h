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

// String functions
void flopstrcopy(char *dst, const char *src, size_t len);
int flopatoi(const char *str);
size_t flopstrlen(const char *str);
int flopstrcmp(const char *s1, const char *s2);
int flopstrncmp(const char *s1, const char *s2, size_t n);
void flopstrrev(char *str);
void flopstrncpy(char *dst, const char *src, size_t n);
void flopstrcat(char *dst, const char *src);
char *flopstrstr(const char *haystack, const char *needle);
char *flopstrrchr(const char *str, int c);
char *flopstrtok(char *str, const char *delim);
char *flopstrchr(const char *str, int c);

// Random number generation
unsigned int floprand(void);
void flopsrand(unsigned int seed);

// Time function
unsigned int floptime(void);

// Integer-to-string conversion
int flopitoa(int value, char *buffer, int width);
int flopitoa_hex(unsigned int value, char *buffer, int width, int is_upper);

// Formatted output
int flopvsnprintf(char *buffer, size_t size, const char *format, va_list args);
int flopsnprintf(char *buffer, size_t size, const char *format, ...);

#endif // STR_H
