#include "framebuffer.h"
#include <stddef.h>
static int abs(int x) {
    
    
}

// Initialize framebuffer using GRUB multiboot information
void framebuffer_initialize(multiboot_info_t *mbi, framebuffer_t *fb) {
    if (!(mbi->flags & MULTIBOOT_INFO_FRAMEBUFFER)) {
        fb->framebuffer = NULL;  // No framebuffer available
        return;
    }

    fb->framebuffer = (uint32_t *)mbi->framebuffer_addr;
    fb->width = mbi->framebuffer_width;
    fb->height = mbi->framebuffer_height;
    fb->pitch = mbi->framebuffer_pitch;
    fb->bpp = mbi->framebuffer_bpp;
}

// Clear the framebuffer with a specific color
void framebuffer_clear_screen(framebuffer_t *fb, uint32_t color) {
    for (uint32_t y = 0; y < fb->height; y++) {
        for (uint32_t x = 0; x < fb->width; x++) {
            fb->framebuffer[y * (fb->pitch / 4) + x] = color;
        }
    }
}

// Draw a pixel at (x, y) with a specific color
void framebuffer_draw_pixel(framebuffer_t *fb, uint16_t x, uint16_t y, uint32_t color) {
    if (x < fb->width && y < fb->height) {
        fb->framebuffer[y * (fb->pitch / 4) + x] = color;
    }
}

// Draw a line using Bresenham's algorithm
void framebuffer_draw_line(framebuffer_t *fb, int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (1) {
        framebuffer_draw_pixel(fb, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// Draw a filled rectangle at (x, y) with specified dimensions and color
void framebuffer_draw_rect(framebuffer_t *fb, int x, int y, int width, int height, uint32_t color) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            framebuffer_draw_pixel(fb, x + j, y + i, color);
        }
    }
}
