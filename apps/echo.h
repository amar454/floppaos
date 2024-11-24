// echo.h
#ifndef ECHO_H
#define ECHO_H

extern unsigned short *terminal_buffer; // Declare as extern
extern unsigned int vga_index; // Declare as extern

void put_char(char c, unsigned char color); // Function to display a character
void read_input(char *buffer, int max_len, unsigned char color); // Function to read input from the user
void echo(const char *str, unsigned char color); // Function to echo a string with color
void echo_address(void *ptr, unsigned char color);
#endif // ECHO_H
