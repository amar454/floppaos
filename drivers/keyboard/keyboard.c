/* 

keyboard.c

Keyboard driver using inb to convert interrupts into text on the screen.

Copyright Amar Djulovic 2024

*/
#include "keyboard.h"
#include "../../apps/echo.h"
#include "../../fshell/fshell.h"
#include "../../fshell/command.h"  // Include the shared command header
#include "../vga/vgacolors.h"
#include <stdint.h>
#include "../io/io.h"

// Define the command buffer
char command[MAX_COMMAND_LENGTH];
int command_ready = 0;  // Initialize the flag as 0 (no command is ready)
// Converts a scancode to an ASCII character, covering basic keys
char key_to_char(unsigned char key) {
    switch (key) {
        case 0x02: return '1';
        case 0x03: return '2';
        case 0x04: return '3';
        case 0x05: return '4';
        case 0x06: return '5';
        case 0x07: return '6';
        case 0x08: return '7';
        case 0x09: return '8';
        case 0x0A: return '9';
        case 0x0B: return '0';
        case 0x10: return 'q';
        case 0x11: return 'w';
        case 0x12: return 'e';
        case 0x13: return 'r';
        case 0x14: return 't';
        case 0x15: return 'y';
        case 0x16: return 'u';
        case 0x17: return 'i';
        case 0x18: return 'o';
        case 0x19: return 'p';
        case 0x1E: return 'a';
        case 0x1F: return 's';
        case 0x20: return 'd';
        case 0x21: return 'f';
        case 0x22: return 'g';
        case 0x23: return 'h';
        case 0x24: return 'j';
        case 0x25: return 'k';
        case 0x26: return 'l';
        case 0x2C: return 'z';
        case 0x2D: return 'x';
        case 0x2E: return 'c';
        case 0x2F: return 'v';
        case 0x30: return 'b';
        case 0x31: return 'n';
        case 0x32: return 'm';
        case 0x39: return ' ';  // Space key
        case 0x0E: return '\b';  // Backspace key
        case 0x1C: return '\n';   // Enter key
        default: return 0;       // Unsupported key
    }
}


// Updated function to check for a scancode without blocking
unsigned char try_read_key(void) {
    if (inb(0x64) & 0x1) {  // Check if the keyboard buffer is not empty
        unsigned char scancode = inb(0x60);
        if (!(scancode & 0x80)) {  // Ignore key releases (high bit set)
            return scancode;
        }
    }
    return 0;  // Return 0 if no valid scancode is available
}

// Non-blocking function to get a character
char try_get_char(void) {
    unsigned char scancode = try_read_key();
    if (scancode != 0) {
        return key_to_char(scancode);
    }
    return 0;  // Return 0 if no character is available
}

// Updated keyboard_task
void keyboard_task(void *arg) {
    static int pos = 0;  // Persistent position in the command buffer

    // Check for a keypress without blocking
    char c = try_get_char();
    if (c == 0) {
        return;  // No keypress, yield back to the scheduler
    } if (c == '\b' && pos > 0) {  // Handle backspace
        pos--;
        vga_index--;               // Move cursor back
        put_char(' ', BLACK);      // Clear character on screen
        vga_index--; 
    } else if (c == '\n') {  // Handle Enter key
        command[pos] = '\0';  // Null-terminate the command
        echo("\n", WHITE);
        command_ready = 1;    // Signal that the command is ready
        pos = 0;              // Reset the buffer position for the next command
    } else if (c >= 32 && c <= 126 && pos < MAX_COMMAND_LENGTH - 1) {  // Handle printable characters
        command[pos++] = c;
        put_char(c, WHITE);   // Echo the character
    }
}






