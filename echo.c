#include "echo.h"
#include "vgacolors.h"
#include "keyboard.h" 
#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_ADDRESS 0xB8000

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

void read_input(char *buffer, int max_len, unsigned char color) {
    int pos = 0;
    while (1) {
        unsigned char key = read_key(); // Read scancode
        char c = key_to_char(key);      // Convert scancode to character

        if (c == '\b' && pos > 0) {     // Handle backspace
            pos--;
            vga_index--;               // Move cursor back
            put_char(' ', color);      // Clear character on screen
            vga_index--;               // Move cursor back again to correct position
        } else if (c == '\n') {        // Enter key
            buffer[pos] = '\0';
            put_char(c, color);        // Move to new line
            break;
        } else if (c >= 32 && c <= 126 && pos < max_len - 1) { // Printable characters
            buffer[pos++] = c;         // Store in buffer
            put_char(c, color);        // Display on screen
        }
    }
}
// Function to echo a string to the terminal with vga color
void echo(const char *str, unsigned char color) {
    while (*str) {
        put_char(*str++, color);
    }
}
