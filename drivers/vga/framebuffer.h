#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>
#include "../../multiboot/multiboot.h"
#include "vgahandler.h"
typedef struct colors {
    uint32_t black;
    uint32_t white;
    uint32_t red;
    uint32_t green;
    uint32_t blue;
    uint32_t yellow;
    uint32_t cyan;
    uint32_t magenta;
    uint32_t gray;
    uint32_t light_gray;
    uint32_t dark_gray;
    uint32_t brown;
    uint32_t orange;
    uint32_t purple;
    uint32_t teal;
    uint32_t navy;
    uint32_t maroon;
    uint32_t olive;
    uint32_t pink;
    uint32_t lime;
    uint32_t aqua;
    uint32_t fuchsia;

} colors_t;

extern colors_t c;
void init_colors(void);
// Function declarations
void framebuffer_init(multiboot_info_t *mbi);
void framebuffer_put_pixel(int x, int y, uint32_t color);
void framebuffer_draw_line(int x1, int y1, int x2, int y2, uint32_t color);
void framebuffer_draw_rectangle(int x, int y, int width, int height, uint32_t color);

void framebuffer_test_triangle();
void framebuffer_test_rectangle();
void framebuffer_test_circle();
void framebuffer_test_pattern();

void framebuffer_draw_char(int x, int y, char c, uint32_t color);
void framebuffer_print_string(int x, int y, const char *str, uint32_t color);

void init_console();
void console_write(const char *str) ;
#endif // FRAMEBUFFER_H