/*
Copyright 2024 Amar Djulovic <aaamargml@gmail.com>

This file is part of FloppaOS.

FloppaOS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

FloppaOS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with FloppaOS. If not, see <https://www.gnu.org/licenses/>.

---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
vgahandler.c

This is the VGA text and graphics handler for FloppaOS. Text output is completely safe and implemented; however, the graphics are very much in a beginning state of development.

Therefore, graphics are not yet accessable to the end user until it can be implemented safely, which is a top priority of development for me right now.

Good luck reading half of this. Most of the memory addresses I found on osdev, so look there for an explanation.
---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
*/

#include "vgahandler.h"
#include "../../drivers/time/floptime.h"
#include "../../drivers/io/io.h"
#include "../../lib/flopmath.h"
#include "framebuffer.h" 
// VGA constants for text mode (in real mode for the time being)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

framebuffer_t fb;
// VGA text buffer
unsigned short *terminal_buffer = (unsigned short *)VGA_TEXT_ADDRESS;
unsigned int vga_index = 0;
// Structure to store terminal state for restoring
static unsigned short *terminal_buffer_saved = NULL;
static uint32_t terminal_color_saved = 0;
static uint16_t terminal_x_saved = 0;
static uint16_t terminal_y_saved = 0;

void vga_clear_terminal() {
    // Clear the entire terminal buffer
    for (unsigned int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        terminal_buffer[i] = (0x07 << 8) | ' '; // Default color (light gray on black) and space character
    }

    // Reset the VGA index and cursor position
    vga_index = 0;
    vga_set_cursor_position(0, 0);
}


// Function to place a character at a specific (x, y) position in the terminal
void vga_place_char(uint16_t x, uint16_t y, char c, uint8_t color) {
    // Check for valid coordinates within the inner area
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
            "mov $0x3D4, %%dx\n\t"         // VGA port for cursor index register
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

void vga_save_terminal_state() {
    // Save current terminal state
    terminal_buffer_saved = terminal_buffer;
    terminal_color_saved = *(uint32_t *)((unsigned char *)terminal_buffer + 1);  // Assuming color is the second byte in each cell
    terminal_x_saved = vga_index % VGA_WIDTH;
    terminal_y_saved = vga_index / VGA_WIDTH;
}

void vga_restore_terminal_state() {
    // Restore terminal buffer to saved state
    if (terminal_buffer_saved != NULL) {
        terminal_buffer = terminal_buffer_saved;
        vga_set_cursor_position(terminal_x_saved, terminal_y_saved); // Restore cursor position
    }
}

// Text-mode function to draw a horizontal line
void textmode_draw_hline(int x, int y, int length, uint32_t color) {
    for (int i = 0; i < length; i++) {
        vga_place_char(x + i, y, '-', color); // Place '-' character for horizontal line
    }
}

// Text-mode function to draw a vertical line
void textmode_draw_vline(int x, int y, int length, uint32_t color) {
    for (int i = 0; i < length; i++) {
        vga_place_char(x, y + i, '|', color); // Place '|' character for vertical line
    }
}

// Text-mode function to draw a rectangle with top-left corner at (x, y), given width and height
void textmode_draw_rectangle(int x, int y, int width, int height, uint32_t color) {
    // Draw the top and bottom lines
    textmode_draw_hline(x, y, width, color); // Top line
    textmode_draw_hline(x, y + height - 1, width, color); // Bottom line

    // Draw the left and right vertical lines
    textmode_draw_vline(x, y, height, color); // Left line
    textmode_draw_vline(x + width - 1, y, height, color); // Right line

    // Fill the inside of the rectangle with spaces (if desired)
    for (int i = 1; i < height - 1; i++) {
        for (int j = 1; j < width - 1; j++) {
            vga_place_char(x + j, y + i, ' ', color); // Fill inside with ' ' (space)
        }
    }
}

// Text-mode function to draw a filled rectangle (border + filled interior)
void textmode_draw_filled_rectangle(int x, int y, int width, int height, uint32_t border_color, uint32_t fill_color) {
    // Draw the top and bottom border
    textmode_draw_hline(x, y, width, border_color); // Top line
    textmode_draw_hline(x, y + height - 1, width, border_color); // Bottom line

    // Draw the left and right vertical borders
    textmode_draw_vline(x, y, height, border_color); // Left line
    textmode_draw_vline(x + width - 1, y, height, border_color); // Right line

    // Fill the interior with fill_color
    for (int i = 1; i < height - 1; i++) {
        for (int j = 1; j < width - 1; j++) {
            vga_place_char(x + j, y + i, ' ', fill_color); // Interior filled with spaces (or use any char)
        }
    }
}

// Text-mode function to draw a diagonal line (from (x0, y0) to (x1, y1))
void textmode_draw_diagonal_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int steps = (dx > dy) ? dx : dy;  // The number of steps for drawing the line

    double x_increment = (double)dx / (double)steps;
    double y_increment = (double)dy / (double)steps;

    double x = x0;
    double y = y0;

    for (int i = 0; i <= steps; i++) {
        vga_place_char((int)x, (int)y, '/', color);  // '/' for diagonal line
        x += x_increment;
        y += y_increment;
    }
}

// Cube vertices (3D space)
double cube_vertices[8][3] = {
    {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
    {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}
};

// Cube edges
int cube_edges[12][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0},
    {4, 5}, {5, 6}, {6, 7}, {7, 4},
    {0, 4}, {1, 5}, {2, 6}, {3, 7}
};

// Projection function
void project(double vertex[3], int *x, int *y, double angle) {
    double sin_angle = sin(angle);
    double cos_angle = cos(angle);

    // Rotate around the Y-axis
    double x_rot = vertex[0] * cos_angle - vertex[2] * sin_angle;
    double z_rot = vertex[0] * sin_angle + vertex[2] * cos_angle;

    // Perspective projection
    double distance = 3.0; // Distance of the viewer from the screen
    double perspective = 1 / (distance - z_rot);

    *x = (int)((double)VGA_WIDTH / 2 + x_rot * perspective * 10);
    *y = (int)((double)VGA_HEIGHT / 2 - vertex[1] * perspective * 10);
}

// Draw spinning cube
void draw_spinning_cube() {
    double angle = 0.0;

    while (1) {
        vga_clear_terminal(); // Clear screen

        // Project and draw edges
        for (int i = 0; i < 12; i++) {
            int start[2], end[2];

            project(cube_vertices[cube_edges[i][0]], &start[0], &start[1], angle);
            project(cube_vertices[cube_edges[i][1]], &end[0], &end[1], angle);

            textmode_draw_diagonal_line(start[0], start[1], end[0], end[1], 0x07);
        }

        angle += 0.05; // Increment angle for rotation

    }
}

// VGA memory address for 256-color mode (0xA0000 to 0xAFFFF)
volatile uint8_t *vga_graphics_buffer = (volatile uint8_t *)VGA_GRAPHICS_ADDRESS;


// Function to write to the VGA sequence port
void write_vga_register(uint16_t port, uint8_t value) {
    __asm__ volatile (
        "outb %0, %1"  // Write value to port
        :
        : "a"(value), "d"(port)
    );
}

// Set VGA Mode (Mode 0x13)
void set_vga_mode() {
    asm volatile ("cli"); // Disable interrupts
    outb(0x3C2, 0x63);    // Set miscellaneous output
    outb(0x3D4, 0x11);    // Unlock CRTC registers
    outb(0x3D5, 0x80);
    outb(0x3D4, 0x00);    // Horizontal total
    outb(0x3D5, 0x5F);
    outb(0x3D4, 0x01);    // End horizontal display
    outb(0x3D5, 0x4F);
    outb(0x3D4, 0x02);    // Start horizontal blanking
    outb(0x3D5, 0x50);
    outb(0x3D4, 0x03);    // End horizontal blanking
    outb(0x3D5, 0x82);
    outb(0x3D4, 0x04);    // Start horizontal sync
    outb(0x3D5, 0x54);
    outb(0x3D4, 0x05);    // End horizontal sync
    outb(0x3D5, 0x80);
    outb(0x3D4, 0x06);    // Vertical total
    outb(0x3D5, 0xBF);
    outb(0x3D4, 0x07);    // Overflow
    outb(0x3D5, 0x1F);
    outb(0x3D4, 0x10);    // Vertical retrace start
    outb(0x3D5, 0x9C);
    outb(0x3D4, 0x11);    // Vertical retrace end
    outb(0x3D5, 0x8E);
    outb(0x3D4, 0x12);    // Vertical display end
    outb(0x3D5, 0x8F);
    outb(0x3D4, 0x13);    // Logical width
    outb(0x3D5, 0x28);
    outb(0x3D4, 0x14);    // Start address high
    outb(0x3D5, 0x40);
    outb(0x3D4, 0x15);    // Start address low
    outb(0x3D5, 0x00);
    outb(0x3D4, 0x16);    // Underline location
    outb(0x3D5, 0x07);
    outb(0x3C0, 0x10);    // Enable VGA graphics
    asm volatile ("sti"); // Enable interrupts
}

// Draw Pixel
void draw_pixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < VGA_GRAPHICS_WIDTH && y >= 0 && y < VGA_GRAPHICS_HEIGHT) {
        vga_graphics_buffer[y * VGA_WIDTH + x] = color;
    }
}

// Fill Screen
void fill_screen(uint8_t color) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_graphics_buffer[i] = color;
    }
}


void disable_vga_cursor() {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
    outb(0x3D4, 0x0B);
    outb(0x3D5, 0x20);
}


// Test Graphics Mode
void test_graphics_mode() {
    set_vga_mode();
    fill_screen(0x0F); // Fill screen with white
    for (int y = 50; y < 150; y++) {
        draw_pixel(100, y, 0x04); // Draw red vertical line
    }
    for (int x = 50; x < 150; x++) {
        draw_pixel(x, 100, 0x01); // Draw blue horizontal line
    }
}