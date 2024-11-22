#include "keyboard.h"
#include "../../apps/echo.h"
#include "../../fshell/fshell.h"
#include "../../fshell/command.h"  // Include the shared command header
#include "../vga/vgacolors.h"
#include <stdint.h>
#include "../io/io.h"

// Modifier key states
static int shift_pressed = 0;
static int ctrl_pressed = 0;
static int alt_pressed = 0;

// Define the command buffer
char command[MAX_COMMAND_LENGTH];
int command_ready = 0;  // Initialize the flag as 0 (no command is ready)

// Converts a scancode to an ASCII character or handles modifiers
char key_to_char(unsigned char key) {
    // Check for modifier keys
    switch (key) {
        case 0x2A: case 0x36:  // Shift pressed
            shift_pressed = 1;
            return 0;
        case 0xAA: case 0xB6:  // Shift released
            shift_pressed = 0;
            return 0;
        case 0x1D:  // Ctrl pressed
            ctrl_pressed = 1;
            return 0;
        case 0x9D:  // Ctrl released
            ctrl_pressed = 0;
            return 0;
        case 0x38:  // Alt pressed
            alt_pressed = 1;
            return 0;
        case 0xB8:  // Alt released
            alt_pressed = 0;
            return 0;
    }

    // Handle regular keys and combinations
    if (key >= 0x02 && key <= 0x35) {
        char c = 0;
        switch (key) {
            // Number keys and symbols
            case 0x02: c = shift_pressed ? '!' : '1'; break;
            case 0x03: c = shift_pressed ? '@' : '2'; break;
            case 0x04: c = shift_pressed ? '#' : '3'; break;
            case 0x05: c = shift_pressed ? '$' : '4'; break;
            case 0x06: c = shift_pressed ? '%' : '5'; break;
            case 0x07: c = shift_pressed ? '^' : '6'; break;
            case 0x08: c = shift_pressed ? '&' : '7'; break;
            case 0x09: c = shift_pressed ? '*' : '8'; break;
            case 0x0A: c = shift_pressed ? '(' : '9'; break;
            case 0x0B: c = shift_pressed ? ')' : '0'; break;
            case 0x0C: c = shift_pressed ? '_' : '-'; break;
            case 0x0D: c = shift_pressed ? '+' : '='; break;

            // Alphabet keys (uppercase with Shift)
            case 0x10: c = shift_pressed ? 'Q' : 'q'; break;
            case 0x11: c = shift_pressed ? 'W' : 'w'; break;
            case 0x12: c = shift_pressed ? 'E' : 'e'; break;
            case 0x13: c = shift_pressed ? 'R' : 'r'; break;
            case 0x14: c = shift_pressed ? 'T' : 't'; break;
            case 0x15: c = shift_pressed ? 'Y' : 'y'; break;
            case 0x16: c = shift_pressed ? 'U' : 'u'; break;
            case 0x17: c = shift_pressed ? 'I' : 'i'; break;
            case 0x18: c = shift_pressed ? 'O' : 'o'; break;
            case 0x19: c = shift_pressed ? 'P' : 'p'; break;
            case 0x1E: c = shift_pressed ? 'A' : 'a'; break;
            case 0x1F: c = shift_pressed ? 'S' : 's'; break;
            case 0x20: c = shift_pressed ? 'D' : 'd'; break;
            case 0x21: c = shift_pressed ? 'F' : 'f'; break;
            case 0x22: c = shift_pressed ? 'G' : 'g'; break;
            case 0x23: c = shift_pressed ? 'H' : 'h'; break;
            case 0x24: c = shift_pressed ? 'J' : 'j'; break;
            case 0x25: c = shift_pressed ? 'K' : 'k'; break;
            case 0x26: c = shift_pressed ? 'L' : 'l'; break;
            case 0x2C: c = shift_pressed ? 'Z' : 'z'; break;
            case 0x2D: c = shift_pressed ? 'X' : 'x'; break;
            case 0x2E: c = shift_pressed ? 'C' : 'c'; break;
            case 0x2F: c = shift_pressed ? 'V' : 'v'; break;
            case 0x30: c = shift_pressed ? 'B' : 'b'; break;
            case 0x31: c = shift_pressed ? 'N' : 'n'; break;
            case 0x32: c = shift_pressed ? 'M' : 'm'; break;

            // Space and symbols
            case 0x27: c = shift_pressed ? ':' : ';'; break;
            case 0x28: c = shift_pressed ? '"' : '\''; break;
            case 0x29: c = shift_pressed ? '~' : '`'; break;
            case 0x2B: c = shift_pressed ? '|' : '\\'; break;
            case 0x33: c = shift_pressed ? '<' : ','; break;
            case 0x34: c = shift_pressed ? '>' : '.'; break;
            case 0x35: c = shift_pressed ? '?' : '/'; break;
            case 0x39: c = ' '; break;  // Space key

            // Backspace and Enter
            case 0x0E: c = '\b'; break;
            case 0x1C: c = '\n'; break;
        }

        // Handle Ctrl, Alt, and combinations
        if (ctrl_pressed || alt_pressed) {
            // Here you can define specific shortcuts (e.g., Ctrl+C)
            // For now, return the base character with Ctrl/Alt metadata
            if (ctrl_pressed && alt_pressed && shift_pressed) {
                echo("[Ctrl+Alt+Shift]", WHITE);
            } else if (ctrl_pressed && alt_pressed) {
                echo("[Ctrl+Alt]", WHITE);
            } else if (ctrl_pressed) {
                echo("[Ctrl]", WHITE);
            } else if (alt_pressed) {
                echo("[Alt]", WHITE);
            }
            return c;
        }

        return c;  // Return the character if no modifier combinations are used
    }

    return 0;  // Unsupported or non-printable key
}
