#ifndef VGAHANDLER_H
#define VGAHANDLER_H

#include <stdint.h>

// VGA constants
#define VGA_ADDRESS 0xB8000
#define VGA_GRAPHICS_ADDRESS 0xA0000
#define VGA_TEXT_MODE 0x03
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_GRAPHICS_MODE 0x13

// VGA color definitions
#define BLACK        0
#define BLUE         1
#define GREEN        2
#define CYAN         3
#define RED          4
#define MAGENTA      5
#define BROWN        6
#define LIGHT_GRAY   7
#define DARK_GRAY    8
#define LIGHT_BLUE   9
#define LIGHT_GREEN  10
#define LIGHT_CYAN   11
#define LIGHT_RED    12
#define LIGHT_MAGENTA 13
#define LIGHT_BROWN  14
#define WHITE        15
#define YELLOW       14

// Function prototypes
void vga_initialize();
void vga_clear();
void vga_switch_to_graphics_mode();
void vga_switch_to_text_mode();
void vga_draw_pixel(uint16_t x, uint16_t y, uint8_t color);
void vga_draw_line(int x0, int y0, int x1, int y1, uint8_t color);
void vga_draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color);
void vga_clear_screen(uint8_t color);

#endif // VGAHANDLER_H
