#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>
#include "../../multiboot/multiboot.h"

// Framebuffer configuration
typedef struct {
    uint32_t *framebuffer;       // Framebuffer address
    uint32_t width;              // Width in pixels
    uint32_t height;             // Height in pixels
    uint32_t pitch;              // Bytes per scanline
    uint32_t bpp;                // Bits per pixel
} framebuffer_t;
// Basic Color Definitions (32-bit RGB)
#define COLOR_BLACK       0x000000
#define COLOR_WHITE       0xFFFFFF
#define COLOR_RED         0xFF0000
#define COLOR_GREEN       0x00FF00
#define COLOR_BLUE        0x0000FF
#define COLOR_CYAN        0x00FFFF
#define COLOR_MAGENTA     0xFF00FF
#define COLOR_YELLOW      0xFFFF00
#define COLOR_GRAY        0x808080
#define COLOR_ORANGE      0xFFA500
#define COLOR_BROWN       0xA52A2A

// Function Prototypes
void framebuffer_initialize(multiboot_info_t *mbi, framebuffer_t *fb);
void framebuffer_clear_screen(framebuffer_t *fb, uint32_t color);
void framebuffer_draw_pixel(framebuffer_t *fb, uint16_t x, uint16_t y, uint32_t color);
void framebuffer_draw_line(framebuffer_t *fb, int x0, int y0, int x1, int y1, uint32_t color);
void framebuffer_draw_rect(framebuffer_t *fb, int x, int y, int width, int height, uint32_t color);
void framebuffer_print_char(framebuffer_t *fb, uint16_t x, uint16_t y, char c, uint32_t color);
void framebuffer_print_string(framebuffer_t *fb, uint16_t x, uint16_t y, const char *str, uint32_t color);
void test_framebuffer(framebuffer_t *fb);
#endif // FRAMEBUFFER_H
