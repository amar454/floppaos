#include "vgahandler.h"
#include "../../lib/flopmath.h"
#include "../../drivers/time/floptime.h"
#include "framebuffer.h" 
// VGA constants for text mode (in real mode for the time being)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

framebuffer_t fb;
// VGA text buffer
unsigned short *terminal_buffer = (unsigned short *)VGA_ADDRESS;
unsigned int vga_index = 0;


void framebuffer_initialize_wrapper(multiboot_info_t *mbi) {
    framebuffer_initialize(mbi, &fb);
    framebuffer_clear_screen(&fb, 0x000000); // Clear screen to black
}

// Clear the VGA screen with a specific color
void vga_clear_screen(uint32_t color) {
    framebuffer_clear_screen(&fb, color);
}


// Draw a pixel at (x, y) with a specific color
void vga_draw_pixel(uint16_t x, uint16_t y, uint32_t color) {
    framebuffer_draw_pixel(&fb, x, y, color);
}
void draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    framebuffer_draw_line(&fb, x0, y0, x1, y1, color);
}

void draw_rectangle(int x, int y, int width, int height, uint32_t color) {
    framebuffer_draw_rect(&fb, x, y, width, height, color);
}
// Function to place a character at a specific (x, y) position in the terminal
void vga_place_char(uint16_t x, uint16_t y, char c, uint8_t color) {
    // Check for valid coordinates
    if (x < VGA_WIDTH && y < VGA_HEIGHT) {
        unsigned int index = y * VGA_WIDTH + x;  // Calculate index based on the position

        // If it's a newline character, move to the next line
        if (c == '\n') {
            // Move to the start of the next line
            y++;
            if (y >= VGA_HEIGHT) {
                // If we're at the bottom of the screen, scroll up
                for (int i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++) {
                    terminal_buffer[i] = terminal_buffer[i + VGA_WIDTH];
                }
                // Clear the last line
                for (int i = VGA_WIDTH * (VGA_HEIGHT - 1); i < VGA_WIDTH * VGA_HEIGHT; i++) {
                    terminal_buffer[i] = (color << 8) | ' ';
                }
                y = VGA_HEIGHT - 1; // Reset to the last line
            }
            x = 0; // Start at the beginning of the new line
        }

        // Set character and color at the calculated position
        terminal_buffer[index] = (color << 8) | (unsigned char)c;

        // If we move past the end of the line, we should handle the newline manually
        if (x >= VGA_WIDTH - 1) {
            x = 0;
            y++;
        }

        // Update the terminal index for next character placement
        if (y >= VGA_HEIGHT) {
            // If we're at the bottom, scroll the screen
            for (int i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++) {
                terminal_buffer[i] = terminal_buffer[i + VGA_WIDTH];
            }
            // Clear the last line
            for (int i = VGA_WIDTH * (VGA_HEIGHT - 1); i < VGA_WIDTH * VGA_HEIGHT; i++) {
                terminal_buffer[i] = (color << 8) | ' ';
            }
            y = VGA_HEIGHT - 1; // Reset to the last line
        }
    }
}

// Set the cursor to a specific position (x, y)
void vga_set_cursor_position(uint16_t x, uint16_t y) {
    if (x < VGA_WIDTH && y < VGA_HEIGHT) {
        unsigned int position = y * VGA_WIDTH + x;  // Calculate linear cursor position

        // Use inline assembly to update VGA cursor position
        asm volatile (
            "mov $0x3D4, %%dx\n\t"          // VGA port for cursor index register
            "mov $0x0F, %%al\n\t"          // Select low cursor byte
            "out %%al, %%dx\n\t"           // Write to index register
            "inc %%dx\n\t"                 // Switch to data port
            "mov %0, %%al\n\t"             // Send low byte of position
            "out %%al, %%dx\n\t"
            "dec %%dx\n\t"                 // Switch back to index register
            "mov $0x0E, %%al\n\t"          // Select high cursor byte
            "out %%al, %%dx\n\t"
            "inc %%dx\n\t"                 // Switch to data port
            "mov %1, %%al\n\t"             // Send high byte of position
            "out %%al, %%dx\n\t"
            :
            : "r"((uint8_t)(position & 0xFF)), "r"((uint8_t)((position >> 8) & 0xFF))
            : "eax", "dx"
        );
    }
}
