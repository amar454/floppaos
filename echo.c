#include "echo.h"
#include "vgacolors.h"
#include "keyboard.h" 
#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_ADDRESS 0xB8000
#define VGA_PORT_COMMAND 0x3D4 // VGA command port for cursor position
#define VGA_PORT_DATA 0x3D5    // VGA data port for cursor position
#define VGA_INDEX_HIGH_BYTE 0x0E // High byte of cursor location
#define VGA_INDEX_LOW_BYTE 0x0F  // Low byte of cursor location
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

void echo_set_cursor_position(uint8_t row, uint8_t col) {
    // Ensure the row and col values are within VGA screen bounds
    if (row >= VGA_HEIGHT || col >= VGA_WIDTH) {
        return; // Exit if the position is out of bounds
    }

    // Calculate the linear offset in the VGA buffer for the given (row, col)
    uint16_t position = row * VGA_WIDTH + col;

    // Set the high byte of the cursor position
    __asm__ volatile (
        "outb %0, %1" 
        : 
        : "a"((uint8_t)VGA_INDEX_HIGH_BYTE), "Nd"(VGA_PORT_COMMAND)
    );
    __asm__ volatile (
        "outb %0, %1" 
        : 
        : "a"((uint8_t)(position >> 8)), "Nd"(VGA_PORT_DATA)
    );

    // Set the low byte of the cursor position
    __asm__ volatile (
        "outb %0, %1" 
        : 
        : "a"((uint8_t)VGA_INDEX_LOW_BYTE), "Nd"(VGA_PORT_COMMAND)
    );
    __asm__ volatile (
        "outb %0, %1" 
        : 
        : "a"((uint8_t)(position & 0xFF)), "Nd"(VGA_PORT_DATA)
    );
}
// Function to echo a string to the terminal with vga color
void echo(const char *str, unsigned char color) {
    while (*str) {
        put_char(*str++, color);
    }
}

void echo_put_at_position(const char *str, uint8_t row, uint8_t col, unsigned char color) {
    // Ensure the row and col values are within VGA screen bounds
    if (row >= VGA_HEIGHT || col >= VGA_WIDTH) {
        return; // Exit if the position is out of bounds
    }

    // Set the cursor position
    echo_set_cursor_position(row, col);

    // Print the string at the specified position
    while (*str) {
        put_char(*str++, color);
    }
}

void echo_clear_screen() {
    // Assuming you have a function to set a pixel or character at a position
    // and know the width and height of the screen (e.g., VGA text mode).
    
    const int SCREEN_WIDTH = 80;  // Example screen width for text mode
    const int SCREEN_HEIGHT = 25;  // Example screen height for text mode

    // Clear the screen by writing spaces or a blank character to each position
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            // Here, you might need to implement your own function to put a character
            // at a specific position (x, y) on the screen. Assuming it's called `put_char`.
            echo_set_cursor_position(x,y);
            put_char(' ', BLACK); // Clear the character at (x, y)
        }
    }

    // Optionally, you can reset the cursor position to the top left after clearing
    echo_set_cursor_position(0,0);
}