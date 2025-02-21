/*

Copyright 2024 Amar Djulovic <aaamargml@gmail.com>

This file is part of FloppaOS.

FloppaOS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

FloppaOS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with FloppaOS. If not, see <https://www.gnu.org/licenses/>.

------------------------------------------------------------------------------

    keyboard.c

        This is the keyboard driver for FloppaOS. It handles keyboard input and keypress events.
        It includes functions to convert scan codes to ASCII characters, read key events, and manage keyboard interrupts.
        The keyboard driver is initialized during system startup and runs as a task to handle keypresses.
        The driver uses a key buffer to store keypresses and provides functions to read characters from the buffer.
        It also supports modifier keys like Shift, Ctrl, and Alt.
        

        - key_to_char(...) converts a keyboard scan code to an ASCII character
        - try_read_key(...) tries to read a key scan code from the keyboard
        - get_char(...) gets a character from the keyboard
        - try_get_char(...) tries to get a character from the keyboard
        - keyboard_isr(...) is the keyboard interrupt service routine
        - init_keyboard(...) initializes the keyboard driver
        - keyboard_task(...) handles keypresses and displays them
    
    keyboard.c 

------------------------------------------------------------------------------

*/

#include "keyboard.h"
#include "../../apps/echo.h"
#include "../../fshell/fshell.h"
#include "../../fshell/command.h" 
#include "../../interrupts/interrupts.h"
#include "../vga/vgahandler.h"
#include "../io/io.h"
#include "../../task/task_handler.h"
#include <stdint.h>
#include <stdbool.h>

// Modifier key states
static int shift_pressed = 0;
static int ctrl_pressed = 0;
static int alt_pressed = 0;

// Define the command buffer
char command[MAX_COMMAND_LENGTH];
int command_ready = 0;  // Initialize the flag as 0 (no command is ready)

// Key buffer for storing keypresses
#define KEY_BUFFER_SIZE 256
char *key_buffer;
int *key_buffer_start;
int *key_buffer_end;

// Flag to signal the keyboard task to wake up
volatile int keyboard_task_wakeup = 0;


const char *key_to_char(unsigned char key) {
    // Check for modifier keys
    switch (key) {
        case 0x2A: case 0x36:  // Shift pressed
            shift_pressed = 1;
            return "";
        case 0xAA: case 0xB6:  // Shift released
            shift_pressed = 0;
            return "";
        case 0x1D:  // Ctrl pressed
            ctrl_pressed = 1;
            return "";
        case 0x9D:  // Ctrl released
            ctrl_pressed = 0;
            return "";
        case 0x38:  // Alt pressed
            alt_pressed = 1;
            return "";
        case 0xB8:  // Alt released
            alt_pressed = 0;
            return "";
    }

    // Handle regular keys and combinations
    if (key >= 0x02 && key <= 0x39) {
        const char *c = "";
        switch (key) {
            // Number keys and symbols
            case 0x02: c = shift_pressed ? "!" : "1"; break;
            case 0x03: c = shift_pressed ? "@" : "2"; break;
            case 0x04: c = shift_pressed ? "#" : "3"; break;
            case 0x05: c = shift_pressed ? "$" : "4"; break;
            case 0x06: c = shift_pressed ? "%" : "5"; break;
            case 0x07: c = shift_pressed ? "^" : "6"; break;
            case 0x08: c = shift_pressed ? "&" : "7"; break;
            case 0x09: c = shift_pressed ? "*" : "8"; break;
            case 0x0A: c = shift_pressed ? "(" : "9"; break;
            case 0x0B: c = shift_pressed ? ")" : "0"; break;
            case 0x0C: c = shift_pressed ? "_" : "-"; break; // Minus
            case 0x0D: c = shift_pressed ? "+" : "="; break; // Equals

            // Alphabet keys
            case 0x10: c = shift_pressed ? "Q" : "q"; break;
            case 0x11: c = shift_pressed ? "W" : "w"; break;
            case 0x12: c = shift_pressed ? "E" : "e"; break;
            case 0x13: c = shift_pressed ? "R" : "r"; break;
            case 0x14: c = shift_pressed ? "T" : "t"; break;
            case 0x15: c = shift_pressed ? "Y" : "y"; break;
            case 0x16: c = shift_pressed ? "U" : "u"; break;
            case 0x17: c = shift_pressed ? "I" : "i"; break;
            case 0x18: c = shift_pressed ? "O" : "o"; break;
            case 0x19: c = shift_pressed ? "P" : "p"; break;
            case 0x1E: c = shift_pressed ? "A" : "a"; break;
            case 0x1F: c = shift_pressed ? "S" : "s"; break;
            case 0x20: c = shift_pressed ? "D" : "d"; break;
            case 0x21: c = shift_pressed ? "F" : "f"; break;
            case 0x22: c = shift_pressed ? "G" : "g"; break;
            case 0x23: c = shift_pressed ? "H" : "h"; break;
            case 0x24: c = shift_pressed ? "J" : "j"; break;
            case 0x25: c = shift_pressed ? "K" : "k"; break;
            case 0x26: c = shift_pressed ? "L" : "l"; break;
            case 0x2C: c = shift_pressed ? "Z" : "z"; break;
            case 0x2D: c = shift_pressed ? "X" : "x"; break;
            case 0x2E: c = shift_pressed ? "C" : "c"; break;
            case 0x2F: c = shift_pressed ? "V" : "v"; break;
            case 0x30: c = shift_pressed ? "B" : "b"; break;
            case 0x31: c = shift_pressed ? "N" : "n"; break;
            case 0x32: c = shift_pressed ? "M" : "m"; break;

            // Symbols and space
            case 0x39: c = " "; break;  // Space key
            case 0x0E: c = "\b"; break; // Backspace
            case 0x1C: c = "\n"; break; // Enter
        }
        return c;
    }

    // Handle special keys
    switch (key) {
        case 0x01: return "Esc";        // Escape key
        case 0x3B: return "F1";         // F1 key
        case 0x3C: return "F2";         // F2 key
        case 0x3D: return "F3";         // F3 key
        case 0x3E: return "F4";         // F4 key
        case 0x3F: return "F5";         // F5 key
        case 0x40: return "F6";         // F6 key
        case 0x41: return "F7";         // F7 key
        case 0x42: return "F8";         // F8 key
        case 0x43: return "F9";         // F9 key
        case 0x44: return "F10";        // F10 key
        case 0x57: return "F11";        // F11 key
        case 0x58: return "F12";        // F12 key
        case 0x0F: return "Tab";        // Tab key
        case 0x35: return "Insert";     // Insert key
        case 0x49: return "Home";       // Home key
        case 0x4B: return "PageUp";     // Page Up key
        case 0x4D: return "Delete";     // Delete key
        case 0x4F: return "End";        // End key
        case 0x51: return "PageDown";   // Page Down key
        case 0x52: return "ArrowUp";    // Up Arrow key
        case 0x53: return "ArrowLeft";  // Left Arrow key
        case 0x54: return "ArrowRight"; // Right Arrow key
        case 0x55: return "ArrowDown";  // Down Arrow key
    }

    return "";  // Unsupported or non-printable key
}

// Function to handle keyboard interrupts
void handle_keyboard_interrupt() {
    unsigned char scancode = inb(0x60); // Read the scancode from the keyboard port
    char c = *key_to_char(scancode); // Convert scancode to character

    if (c != 0) {
        // Store the character in the key buffer
        key_buffer[*key_buffer_end] = c;
        // Update the key buffer end index
        *key_buffer_end = (*key_buffer_end + 1) % KEY_BUFFER_SIZE; 
    }

    // Signal the keyboard task to wake up
    keyboard_task_wakeup = 1;
}

// Keyboard task to handle keypresses and display them
void keyboard_task(void *arg) {
    Task *task = (Task *)arg;

    if (!key_buffer) { // alloc memory for the key buffer
        key_buffer = (char *)task_alloc_memory(task, KEY_BUFFER_SIZE);
        key_buffer_start = (int *)task_alloc_memory(task, sizeof(int));
        key_buffer_end = (int *)task_alloc_memory(task, sizeof(int));

        // Initialize buffer indices
        *key_buffer_start = 0;
        *key_buffer_end = 0;
    }

    static int pos = 0; 

    while (1) {
        if (!keyboard_task_wakeup) {
            break;
        }

        // Reset the wakeup flag
        keyboard_task_wakeup = 0;

        while (*key_buffer_start != *key_buffer_end) {
            char c = key_buffer[*key_buffer_start];
            *key_buffer_start = (*key_buffer_start + 1) % KEY_BUFFER_SIZE;

            if (c == '\b' && pos > 0) {   // Backspace
                pos--;                    
                vga_index--;              // Move cursor back
                put_char(' ', BLACK);     // Clear character
                vga_index--; 
            } else if (c == '\n') {       // Enter key
                command[pos] = '\0';      // Null-terminate command
                echo("\n", WHITE);
                command_ready = 1;        // Signal that command is ready to be parsed (hook for fshell in ../../fshell/command.h)
                pos = 0;                  // Reset buffer position
            } else if (c >= 32 && c <= 126 && pos < MAX_COMMAND_LENGTH - 1) { 
                command[pos++] = c;
                put_char(c, WHITE);
            }
        }
    }
}

// Initialize the keyboard driver
void init_keyboard() {
    // Add the keyboard task
    add_task(keyboard_task, NULL, 1, "keyboard", "floppaos://drivers/keyboard/keyboard.c");
}

// Keyboard ISR handler in C
__attribute__((interrupt)) void keyboard_isr(interrupt_frame_t* frame) {
    // Call the keyboard interrupt handler
    handle_keyboard_interrupt(); 

    // Signal a context switch is needed
    schedule_needed = 1;

    // Send End of Interrupt (EOI) to PIC
    outb(0x20, 0x20);  // Send EOI to PIC1
}