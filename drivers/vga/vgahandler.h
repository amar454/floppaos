#ifndef VGAHANDLER_H
#define VGAHANDLER_H

#include <stdint.h>
#include <stdint.h>
#include "../../lib/flopmath.h"
#include "../../drivers/time/floptime.h"
#include "framebuffer.h"
// VGA Constants
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
    // ASCII box-drawing characters

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

// VGA text buffer address
#define VGA_ADDRESS 0xB8000

// External framebuffer structure
extern framebuffer_t fb;

// VGA functions
void vga_clear_terminal(void);
void framebuffer_initialize_wrapper(multiboot_info_t *mbi);
void vga_clear_screen(uint32_t color);
void vga_draw_pixel(uint16_t x, uint16_t y, uint32_t color);
void draw_line(int x0, int y0, int x1, int y1, uint32_t color);
void draw_rectangle(int x, int y, int width, int height, uint32_t color);
void vga_place_char(uint16_t x, uint16_t y, char c, uint8_t color);
void vga_set_cursor_position(uint16_t x, uint16_t y);
void vga_save_terminal_state(void);
void vga_restore_terminal_state(void);
void textmode_draw_hline(int x, int y, int length, uint32_t color);
void textmode_draw_vline(int x, int y, int length, uint32_t color);
void textmode_draw_rectangle(int x, int y, int width, int height, uint32_t color);
void textmode_draw_filled_rectangle(int x, int y, int width, int height, uint32_t border_color, uint32_t fill_color);
void textmode_draw_diagonal_line(int x0, int y0, int x1, int y1, uint32_t color);
void draw_spinning_cube(void);

#endif // VGAHANDLER_H