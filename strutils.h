#ifndef STRUTILS_H
#define STRUTILS_H

#include <stddef.h> // For size_t

// Function declarations
void flopstrcopy(char *dest, const char *src);
size_t flopstrlen(const char *str);
int flopstrcmp(const char *s1, const char *s2);
int flopstrncmp(const char *s1, const char *s2, size_t n);
unsigned int floprand(void);
void flopsrand(unsigned int seed);
unsigned int floptime(void); // New function declaration for time
char *flopstrtok(char *str, const char *delim); // Declaration for flopstrtok
char *flopstrchr(const char *str, int c);
#endif // STRUTILS_H
