#ifndef VGAHANDLER_H
#define VGAHANDLER_H

#include <stdint.h>
// VGA Constants
#define VGA_ADDRESS 0xB8000
#define VGA_GRAPHICS_ADDRESS 0xA0000
#define VGA_TEXT_MODE 0x03
#define VGA_GRAPHICS_MODE 0x13
#define VGA_GRAPHICS_WIDTH 320
#define VGA_GRAPHICS_HEIGHT 200
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define TERMINAL_BUFFER_SIZE (VGA_WIDTH * VGA_HEIGHT + 1)
// VGA Color Definitions
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

// Function Prototypes
void vga_initialize();
void vga_clear_screen(uint32_t color);
void vga_place_char(uint16_t x, uint16_t y, char c, uint8_t color);
void vga_set_cursor_position(uint16_t x, uint16_t y);
void vga_draw_pixel(uint16_t x, uint16_t y, uint32_t color);
void vga_draw_line(int x0, int y0, int x1, int y1, uint8_t color);
void vga_draw_circle(uint16_t center_x, uint16_t center_y, uint16_t radius, uint8_t color);
void draw_circle_callback();
void draw_circle_for_10_seconds();
#endif // VGAHANDLER_H
