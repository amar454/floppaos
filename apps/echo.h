// echo.h
#ifndef ECHO_H
#define ECHO_H

#include <stdint.h>
extern unsigned short *terminal_buffer; 
extern int vga_index; 
void restore_terminal_buffer(const char *buffer, const uint8_t *colors);
void put_char(char c, unsigned char color); // Function to display a character
void echo_bold(const char *str, unsigned char color) ;
void echo(const char *str, unsigned char color) ;
void echo_f(const char *format, int color, ...);
void echo_address(void *ptr, unsigned char color);
#endif // ECHO_H
