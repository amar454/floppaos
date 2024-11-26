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

Good luck reading half of this. 
---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
*/

#include "vgahandler.h"
#include "../../lib/flopmath.h"

static uint8_t *graphics_buffer = (uint8_t *)VGA_GRAPHICS_ADDRESS;
// VGA text buffer
unsigned short *terminal_buffer = (unsigned short *)VGA_ADDRESS;
unsigned int vga_index = 0;

// Initialize VGA graphics mode and text mode
void vga_initialize() {
    // You can put specific initialization code here if needed, like setting modes.
    vga_switch_to_graphics_mode();
}

// Clear the VGA screen with a specific color
void vga_clear_screen(uint8_t color) {
    for (int i = 0; i < 320 * 200; i++) {
        graphics_buffer[i] = color;
    }
}

// Switch to VGA graphics mode (Mode 0x13)
void vga_switch_to_graphics_mode() {
    asm volatile (
        "movb $0x13, %%al\n\t"  // Set video mode to 0x13 (320x200, 256 colors)
        "int $0x10"             // BIOS video interrupt
        :
        :
        : "eax"
    );
}

// Switch to text mode (Mode 0x03)
void vga_switch_to_text_mode() {
    asm volatile (
        "movb $0x03, %%al\n\t"  // Set video mode to 0x03 (80x25, 16 colors)
        "int $0x10"             // BIOS video interrupt
        :
        :
        : "eax"
    );
}

// Draw a pixel at (x, y) with a specified color
void vga_draw_pixel(uint16_t x, uint16_t y, uint8_t color) {
    if (x < 320 && y < 200) {
        graphics_buffer[y * 320 + x] = color;
    }
}

// Draw a line using Bresenham's algorithm
void vga_draw_line(int x0, int y0, int x1, int y1, uint8_t color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (1) {
        vga_draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// Draw a filled triangle
void vga_draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color) {
    vga_draw_line(x0, y0, x1, y1, color);
    vga_draw_line(x1, y1, x2, y2, color);
    vga_draw_line(x2, y2, x0, y0, color);
}
