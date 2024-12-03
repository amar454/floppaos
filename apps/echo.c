#include "echo.h"
#include "../drivers/vga/vgahandler.h"
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include "../lib/str.h"
#define VGA_ADDRESS 0xB8000
// Function to retrieve all characters in the terminal buffer
void get_terminal_content(char *buffer, size_t buffer_size) {
    unsigned int index = 0;

    // Ensure the buffer has enough space to store characters, plus a null terminator
    if (buffer_size < VGA_WIDTH * VGA_HEIGHT + 1) {
        return; // Not enough space in the provided buffer
    }

    // Iterate through the terminal buffer to extract characters
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        unsigned short entry = terminal_buffer[i];
        char character = entry & 0xFF; // Extract the lower byte (character)
        
        // Store the character in the output buffer
        buffer[index++] = character;
    }

    // Null-terminate the string
    buffer[index] = '\0';
}

// Function to set a character in the terminal at the current vga_index
void put_char(char c, unsigned char color) {
    // Check for newline character and handle it
    if (c == '\n') {
        vga_index += VGA_WIDTH - (vga_index % VGA_WIDTH);
    } else {
        // Set character and color at the current index in terminal buffer
        terminal_buffer[vga_index++] = (color << 8) | (unsigned char)c;
    }

    // Scroll the screen if necessary
    if (vga_index >= VGA_WIDTH * VGA_HEIGHT) {
        // Shift all lines up by one line
        for (int i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++) {
            terminal_buffer[i] = terminal_buffer[i + VGA_WIDTH];
        }
        // Clear the last line
        for (int i = VGA_WIDTH * (VGA_HEIGHT - 1); i < VGA_WIDTH * VGA_HEIGHT; i++) {
            terminal_buffer[i] = (color << 8) | ' ';
        }
        vga_index -= VGA_WIDTH; // Move index to the start of the last line
    }
}


// Function to echo a string to the terminal with vga color
void echo(const char *str, unsigned char color) {
    while (*str) {
        put_char(*str++, color);
    }
}

void echo_f(const char *format, unsigned char color, ...) {
    va_list args;
    va_start(args, color);

    char buffer[128];  // Temporary buffer for formatted output
    int buffer_index = 0;

    for (const char *ptr = format; *ptr != '\0'; ++ptr) {
        if (*ptr == '%' && *(ptr + 1) != '\0') {
            ++ptr;
            switch (*ptr) {
                case 'd': {  // Integer
                    int num = va_arg(args, int);
                    if (num < 0) {
                        buffer[buffer_index++] = '-';
                        num = -num;
                    }
                    char num_buffer[16];
                    int num_len = 0;
                    do {
                        num_buffer[num_len++] = '0' + (num % 10);
                        num /= 10;
                    } while (num > 0);
                    for (int i = num_len - 1; i >= 0; --i) {
                        buffer[buffer_index++] = num_buffer[i];
                    }
                    break;
                }
                case 'u': {  // Unsigned integer
                    unsigned int num = va_arg(args, unsigned int);
                    char num_buffer[16];
                    int num_len = 0;
                    do {
                        num_buffer[num_len++] = '0' + (num % 10);
                        num /= 10;
                    } while (num > 0);
                    for (int i = num_len - 1; i >= 0; --i) {
                        buffer[buffer_index++] = num_buffer[i];
                    }
                    break;
                }
                case 'x': {  // Hexadecimal
                    unsigned int num = va_arg(args, unsigned int);
                    char hex_chars[] = "0123456789ABCDEF";
                    char num_buffer[16];
                    int num_len = 0;
                    do {
                        num_buffer[num_len++] = hex_chars[num % 16];
                        num /= 16;
                    } while (num > 0);
                    for (int i = num_len - 1; i >= 0; --i) {
                        buffer[buffer_index++] = num_buffer[i];
                    }
                    break;
                }
                case 's': {  // String
                    const char *str = va_arg(args, const char *);
                    while (*str) {
                        buffer[buffer_index++] = *str++;
                    }
                    break;
                }
                case '%': {  // Literal '%'
                    buffer[buffer_index++] = '%';
                    break;
                }
                default: {
                    // Unknown specifier; ignore or handle as needed
                    break;
                }
            }
        } else {
            buffer[buffer_index++] = *ptr;
        }

        // If the buffer is full, flush it
        if (buffer_index >= sizeof(buffer) - 1) {
            buffer[buffer_index] = '\0';
            echo(buffer, color);
            buffer_index = 0;
        }
    }

    // Flush remaining buffer
    if (buffer_index > 0) {
        buffer[buffer_index] = '\0';
        echo(buffer, color);
    }

    va_end(args);
}

void retrieve_terminal_buffer(char *buffer, uint8_t *colors) {
    const unsigned short *terminal_buffer = (unsigned short *)VGA_ADDRESS;

    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        unsigned short entry = terminal_buffer[i];
        
        // Extract the character (lower 8 bits)
        buffer[i] = (char)(entry & 0xFF);
        
        // Extract the color (upper 8 bits)
        colors[i] = (uint8_t)((entry >> 8) & 0xFF);
    }
}

// Function to restore characters to the terminal buffer
void restore_terminal_buffer(const char *buffer, const uint8_t *colors) {
    unsigned short *terminal_buffer = (unsigned short *)VGA_ADDRESS;

    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        if (buffer[i] == '\0') {
            // If the buffer contains a null character, treat it as a space
            terminal_buffer[i] = (colors[i] << 8) | ' ';
        } else {
            // Restore character and color
            terminal_buffer[i] = (colors[i] << 8) | buffer[i];
        }
    }
}