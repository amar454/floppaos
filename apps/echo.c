#include "echo.h"
#include "../drivers/vga/vgahandler.h"
#include "../drivers/vga/framebuffer.h"
#include "../flanterm/src/flanterm.h"
#include "../flanterm/src/flanterm_backends/fb.h"
#include "../lib/str.h"
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
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
    console_write(str); // Use console_write to output the string
}

void echo_bold(const char *str, unsigned char color) {
    // Apply bold effect by setting the intensity bit (4th bit)
    color = color | 0x08;

    // Output the string character by character
    while (*str) {
        put_char(*str++, color);
    }
}

void echo_f(const char *format, int color, ...) {
    char buffer[256];  // Fixed buffer size
    va_list args;

    va_start(args, color);

    // Use flopsnprintf to format the string
    int result = flopsnprintf(buffer, sizeof(buffer), format, args);

    va_end(args);

    // Check the result from flopsnprintf
    if (result < 0) {
        buffer[0] = '\0';  // Ensure the buffer is empty on failure
    } else if ((size_t)result >= sizeof(buffer)) {
        buffer[sizeof(buffer) - 1] = '\0';  // Ensure null-termination on truncation
    }

    // Output the formatted string using the echo function
    echo(buffer, color);
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