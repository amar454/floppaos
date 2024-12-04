#ifndef STR_H
#define STR_H

#include <stddef.h> // For size_t

// String manipulation functions
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
int flopitoa(int value, char *buffer, int width);
int flopvsnprintf(char *buffer, size_t size, const char *format, va_list args);
int flopsnprintf(char *buffer, size_t size, const char *format, ...);

// Random number generation
unsigned int floprand(void);
void flopsrand(unsigned int seed);

// Time function
unsigned int floptime(void);

#endif // STR_H
