#ifndef VGACOLORS_H
#define VGACOLORS_H

#include <stdint.h>

#define VGA_ADDRESS 0xB8000

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

extern unsigned short *terminal_buffer; // Declaration only, no definition here
extern unsigned int vga_index;          // Declaration only, no definition here

#endif // VGACOLORS_H
