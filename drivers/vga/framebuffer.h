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

// Function Prototypes
void framebuffer_initialize(multiboot_info_t *mbi, framebuffer_t *fb);
void framebuffer_clear_screen(framebuffer_t *fb, uint32_t color);
void framebuffer_draw_pixel(framebuffer_t *fb, uint16_t x, uint16_t y, uint32_t color);
void framebuffer_draw_line(framebuffer_t *fb, int x0, int y0, int x1, int y1, uint32_t color);
void framebuffer_draw_rect(framebuffer_t *fb, int x, int y, int width, int height, uint32_t color);

#endif // FRAMEBUFFER_H
