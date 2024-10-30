// echo.h
#ifndef ECHO_H
#define ECHO_H
#include <stdint.h>
extern unsigned short *terminal_buffer; // Declare as extern
extern unsigned int vga_index; // Declare as extern

void put_char(char c, unsigned char color); // Function to display a character
void read_input(char *buffer, int max_len, unsigned char color); // Function to read input from the user
void echo(const char *str, unsigned char color); // Function to echo a string with color
void echo_set_cursor_position(uint8_t row, uint8_t col);
void echo_put_at_position(const char *str, uint8_t row, uint8_t col, unsigned char color);
void echo_clear_screen();
#endif // ECHO_H
