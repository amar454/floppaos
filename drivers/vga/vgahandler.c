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



// my girlfriend Jaden says hi :)



#include "vgahandler.h"
#include "../../drivers/time/floptime.h"
#include "../../drivers/io/io.h"
#include "../../lib/flopmath.h"
#include "framebuffer.h" 
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#include "../../lib/str.h"
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
void vga_place_string(uint16_t x, uint16_t y, const char *str, uint8_t color) { 
    for (int i = 0; i < flopstrlen(str); i++) {
        vga_place_char(x + i, y, str[i], color);
    }
}
// Function to place a bold character at a specific (x, y) position in the terminal
void vga_place_bold_char(uint16_t x, uint16_t y, char c, uint8_t color) {
    // Check for valid coordinates within the terminal boundaries
    if (x < VGA_WIDTH && y < VGA_HEIGHT) {
        unsigned int index = y * VGA_WIDTH + x;  // Calculate index based on position

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

        // Set the character at the (x, y) position
        terminal_buffer[index] = (color << 8) | (unsigned char)c;

        if (x + 1 < VGA_WIDTH) {
            terminal_buffer[index + 1] = (color << 8) | (unsigned char)c;
        }

        // If we move past the end of the line, manually handle the newline
        if (x >= VGA_WIDTH - 1) {
            x = 0;
            y++;
        }

        // Update the terminal index for the next character
        if (y >= VGA_HEIGHT) {
            // Scroll the screen if we are at the bottom
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
void vga_set_cursor_position(uint16_t x, uint16_t y) {
    if (x < VGA_WIDTH && y < VGA_HEIGHT) {
        uint16_t position = y * VGA_WIDTH + x;  // calc linear cursor pos.

        // Set high byte 
        outb(VGA_CRTC_INDEX, VGA_CURSOR_HIGH_REGISTER);
        outb(VGA_CRTC_DATA, (position >> 8) & 0xFF);

        // Set low byte
        outb(VGA_CRTC_INDEX, VGA_CURSOR_LOW_REGISTER);
        outb(VGA_CRTC_DATA, position & 0xFF);
    }
}
#define VGA_CURSOR_COLOR_REGISTER 0x3D5
void vga_set_foreground_color(int x ,int y, uint8_t color) {
    if (x < VGA_WIDTH && y < VGA_HEIGHT) {
        uint16_t position = y * VGA_WIDTH + x;  // calc linear cursor pos.

        // Set high byte 
        outb(VGA_CRTC_INDEX, VGA_CURSOR_HIGH_REGISTER);
        outb(VGA_CRTC_DATA, (position >> 8) & 0xFF);

        // Set low byte
        outb(VGA_CRTC_INDEX, VGA_CURSOR_LOW_REGISTER);
        outb(VGA_CRTC_DATA, position & 0xFF);

        // Set color
        outb(VGA_CURSOR_COLOR_REGISTER, color);
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




typedef struct {
    const char *name;
    unsigned char r, g, b;
} VGAColor;

const VGAColor vga_palette[256] = {
    {"Black", 0x00, 0x00, 0x00},            // 0
    {"Blue", 0x00, 0x00, 0xAA},            // 1
    {"Green", 0x00, 0xAA, 0x00},           // 2
    {"Cyan", 0x00, 0xAA, 0xAA},            // 3
    {"Red", 0xAA, 0x00, 0x00},             // 4
    {"Magenta", 0xAA, 0x00, 0xAA},         // 5
    {"Brown", 0xAA, 0x55, 0x00},           // 6
    {"Light Gray", 0xAA, 0xAA, 0xAA},      // 7
    {"Dark Gray", 0x55, 0x55, 0x55},       // 8
    {"Bright Blue", 0x55, 0x55, 0xFF},     // 9
    {"Bright Green", 0x55, 0xFF, 0x55},    // 10
    {"Bright Cyan", 0x55, 0xFF, 0xFF},     // 11
    {"Bright Red", 0xFF, 0x55, 0x55},      // 12
    {"Bright Magenta", 0xFF, 0x55, 0xFF},  // 13
    {"Yellow", 0xFF, 0xFF, 0x55},          // 14
    {"White", 0xFF, 0xFF, 0xFF},           // 15
    {"Maroon", 0x80, 0x00, 0x00},          // 16
    {"Dark Green", 0x00, 0x80, 0x00},      // 17
    {"Navy", 0x00, 0x00, 0x80},            // 18
    {"Olive", 0x80, 0x80, 0x00},           // 19
    {"Teal", 0x00, 0x80, 0x80},            // 20
    {"Purple", 0x80, 0x00, 0x80},          // 21
    {"Silver", 0xC0, 0xC0, 0xC0},          // 22
    {"Gray", 0x80, 0x80, 0x80},            // 23
    {"Orange", 0xFF, 0xA5, 0x00},          // 24
    {"Pink", 0xFF, 0xC0, 0xCB},            // 25
    {"Gold", 0xFF, 0xD7, 0x00},            // 26
    {"Beige", 0xF5, 0xF5, 0xDC},           // 27
    {"Lavender", 0xE6, 0xE6, 0xFA},        // 28
    {"Sky Blue", 0x87, 0xCE, 0xEB},        // 29
    {"Coral", 0xFF, 0x7F, 0x50},           // 30
    {"Turquoise", 0x40, 0xE0, 0xD0},       // 31
    {"Sea Green", 0x2E, 0x8B, 0x57},       // 32
    {"Slate Gray", 0x70, 0x80, 0x90},      // 33
    {"Rosy Brown", 0xBC, 0x8F, 0x8F},      // 34
    {"Peru", 0xCD, 0x85, 0x3F},            // 35
    {"Chocolate", 0xD2, 0x69, 0x1E},       // 36
    {"Tan", 0xD2, 0xB4, 0x8C},             // 37
    {"Plum", 0xDD, 0xA0, 0xDD},            // 38
    {"Khaki", 0xF0, 0xE6, 0x8C},           // 39
    {"Mint Cream", 0xF5, 0xFF, 0xFA},      // 40
    {"Indigo", 0x4B, 0x00, 0x82},          // 41
    {"Chartreuse", 0x7F, 0xFF, 0x00},      // 42
    {"Lime Green", 0x32, 0xCD, 0x32},      // 43
    {"Dodger Blue", 0x1E, 0x90, 0xFF},     // 44
    {"Dark Orange", 0xFF, 0x8C, 0x00},     // 45
    {"Violet", 0xEE, 0x82, 0xEE},          // 46
    {"Tomato", 0xFF, 0x63, 0x47},          // 47
    {"Crimson", 0xDC, 0x14, 0x3C},         // 48
    {"Medium Orchid", 0xBA, 0x55, 0xD3},   // 49
    {"Dark Olive Green", 0x55, 0x6B, 0x2F},// 50
};

const uint8_t SMALL_FONT[95][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // Space :32
    {0x0C, 0x0C, 0x00, 0x0C, 0x0C}, // ! :33
    {0x00, 0x00, 0x00, 0x00, 0x00}, // " :34
    {0x1E, 0x1E, 0x1E, 0x1E, 0x1E}, // # :35
    {0x1E, 0x3F, 0x37, 0x0E, 0x1E}, // $ :36
    {0x33, 0x33, 0x1E, 0x33, 0x33}, // % :37
    {0x1E, 0x33, 0x33, 0x33, 0x1E}, // & :38
    {0x00, 0x00, 0x00, 0x00, 0x00}, // ' :39
    {0x06, 0x0C, 0x18, 0x0C, 0x06}, // ( :40
    {0x18, 0x0C, 0x06, 0x0C, 0x18}, // ) :41
    {0x1E, 0x0C, 0x1E, 0x0C, 0x1E}, // * :42
    {0x06, 0x06, 0x1E, 0x06, 0x06}, // + :43
    {0x00, 0x00, 0x00, 0x00, 0x06}, // , :44
    {0x00, 0x00, 0x1E, 0x00, 0x00}, // - :45
    {0x00, 0x00, 0x00, 0x00, 0x00}, // . :46
    {0x03, 0x06, 0x0C, 0x18, 0x30}, // / :47
    {0x1E, 0x33, 0x33, 0x33, 0x1E}, // 0 :48
    {0x06, 0x0E, 0x06, 0x06, 0x1F}, // 1 :49
    {0x1E, 0x03, 0x1E, 0x30, 0x1E}, // 2 :50
    {0x1E, 0x03, 0x1E, 0x03, 0x1E}, // 3 :51
    {0x33, 0x33, 0x1F, 0x03, 0x03}, // 4 :52
    {0x1F, 0x30, 0x1E, 0x03, 0x1E}, // 5 :53
    {0x1E, 0x30, 0x1E, 0x33, 0x1E}, // 6 :54
    {0x1F, 0x03, 0x03, 0x03, 0x03}, // 7 :55
    {0x1E, 0x33, 0x1E, 0x33, 0x1E}, // 8 :56
    {0x1E, 0x33, 0x1E, 0x03, 0x1E}, // 9 :57
    {0x00, 0x00, 0x00, 0x00, 0x00}, // :58
    {0x00, 0x00, 0x00, 0x00, 0x06}, // ; :59
    {0x03, 0x06, 0x0C, 0x18, 0x30}, // < :60
    {0x1E, 0x00, 0x1E, 0x00, 0x1E}, // = :61
    {0x30, 0x18, 0x0C, 0x06, 0x03}, // > :62
    {0x1E, 0x03, 0x1E, 0x00, 0x00}, // ? :63
    {0x1E, 0x33, 0x33, 0x37, 0x1E}, // @ :64
    {0x33, 0x33, 0x1F, 0x33, 0x33}, // A :65
    {0x1E, 0x33, 0x1E, 0x33, 0x1E}, // B :66
    {0x1E, 0x33, 0x33, 0x33, 0x1E}, // C :67
    {0x1E, 0x33, 0x33, 0x33, 0x1E}, // D :68
    {0x1F, 0x30, 0x1F, 0x30, 0x1F}, // E :69
    {0x1F, 0x30, 0x1F, 0x30, 0x30}, // F :70
    {0x1E, 0x33, 0x33, 0x33, 0x1F}, // G :71
    {0x33, 0x33, 0x1F, 0x33, 0x33}, // H :72
    {0x1E, 0x06, 0x06, 0x06, 0x1E}, // I :73
    {0x33, 0x33, 0x33, 0x33, 0x1E}, // J :74
    {0x33, 0x33, 0x1F, 0x33, 0x33}, // K :75
    {0x30, 0x30, 0x30, 0x30, 0x1F}, // L :76
    {0x33, 0x33, 0x33, 0x33, 0x33}, // M :77
    {0x33, 0x33, 0x33, 0x33, 0x33}, // N :78
    {0x1E, 0x33, 0x33, 0x33, 0x1E}, // O :79
    {0x1E, 0x33, 0x1E, 0x03, 0x03}, // P :80
    {0x1E, 0x33, 0x33, 0x33, 0x1E}, // Q :81
    {0x1E, 0x33, 0x1E, 0x33, 0x33}, // R :82
    {0x1E, 0x30, 0x1E, 0x03, 0x1E}, // S :83
    {0x1F, 0x06, 0x06, 0x06, 0x06}, // T :84
    {0x33, 0x33, 0x33, 0x33, 0x33}, // U :85
    {0x33, 0x33, 0x33, 0x33, 0x33}, // V :86
    {0x33, 0x33, 0x33, 0x33, 0x33}, // W :87
    {0x33, 0x33, 0x33, 0x33, 0x33}, // X :88
    {0x33, 0x33, 0x33, 0x33, 0x33}, // Y :89
    {0x33, 0x33, 0x33, 0x33, 0x33}, // Z :90
    {0x00, 0x00, 0x00, 0x00, 0x00}, // [ :91
    {0x00, 0x00, 0x00, 0x00, 0x00}, // \ :92
    {0x00, 0x00, 0x00, 0x00, 0x00}, // ] :93
    {0x00, 0x00, 0x00, 0x00, 0x00}, // ^ :94
    {0x00, 0x00, 0x00, 0x00, 0x00}  // _ :95
};

unsigned char g_320x200x256[] = // mapping crap.
{
	0x63,
	0x03, 0x01, 0x0F, 0x00, 0x0E,
	0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
	0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x9C, 0x0E, 0x8F, 0x28,	0x40, 0x96, 0xB9, 0xA3,
	0xFF,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,
	0xFF,
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x41, 0x00, 0x0F, 0x00,	0x00
};


void vga_plot_pixel(int x, int y, unsigned short color) {
    unsigned short offset = x + 320 * y;
    unsigned char *VGA = (unsigned char*)VGA_GRAPHICS_ADDRESS;
    VGA[offset] = color;
}


void set_palette() {
    outb(0x3C8, 0); // Start at color 0
    for (int i = 0; i < 256; i++) {
        // Retrieve the RGB values from the predefined vga_palette
        unsigned char r = vga_palette[i].r;
        unsigned char g = vga_palette[i].g;
        unsigned char b = vga_palette[i].b;

        // Scale the values from 0-255 to the VGA's 6-bit range (0-63)
        outb(0x3C9, r / 4); // VGA expects values in the range 0-63
        outb(0x3C9, g / 4);
        outb(0x3C9, b / 4);
    }
}

void vga_draw_char(int x, int y, char c, uint32_t color) {
    if (c < 32 || c > 126) {
        return;  // Invalid character (outside the printable range)
    }

    const uint8_t *bitmap;
    for (int row = 0; row < 13; row++) {  // Iterate over 13 rows
        for (int col = 0; col < 8; col++) {  // Iterate over 8 columns
            if (bitmap[12 - row] & (0x80 >> col)) {  // Reverse the row to correct the upside-down issue
                vga_plot_pixel(x + col, y + row, color);  // Plot the pixel
            }
        }
    }
}

void vga_draw_string(int x, int y, const char *str, uint32_t color) {
    int offset_x = 0;
    while (*str) {
        vga_draw_char(x + offset_x, y, *str, color);  // Draw each character
        offset_x += 9;  // Move the position for the next character, scaled width 
        str++;
    }
}

void write_regs(unsigned char *regs) {
    unsigned i;

    outb(VGA_MISC_WRITE, *regs);
    regs++;
    for (i = 0; i < VGA_NUM_SEQ_REGS; i++) {
        outb(VGA_SEQ_INDEX, i);
        outb(VGA_SEQ_DATA, *regs);
        regs++;
    }
    outb(VGA_CRTC_INDEX, 0x03);
    outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA) | 0x80);
    outb(VGA_CRTC_INDEX, 0x11);
    outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA) & ~0x80);
    regs[0x03] |= 0x80;
    regs[0x11] &= ~0x80;
    for (i = 0; i < VGA_NUM_CRTC_REGS; i++) {
        outb(VGA_CRTC_INDEX, i);
        outb(VGA_CRTC_DATA, *regs);
        regs++;
    }
    for (i = 0; i < VGA_NUM_GC_REGS; i++) {
        outb(VGA_GC_INDEX, i);
        outb(VGA_GC_DATA, *regs);
        regs++;
    }

    for (i = 0; i < VGA_NUM_AC_REGS; i++) {
        (void)inb(VGA_INSTAT_READ);
        outb(VGA_AC_INDEX, i);
        outb(VGA_AC_WRITE, *regs);
        regs++;
    }
    (void)inb(VGA_INSTAT_READ);
    outb(VGA_AC_INDEX, 0x20);
}
void draw_filled_rectangle(int x, int y, int width, int height, unsigned short color) {
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			vga_plot_pixel(x+i, y+j, color);
		}
	}
}
void draw_rectangle(int x, int y, int width, int height, unsigned short color) {
    // Draw top and bottom borders
    for (int i = 0; i < width; i++) {
        vga_plot_pixel(x + i, y, color);           // Top border
        vga_plot_pixel(x + i, y + height - 1, color); // Bottom border
    }

    // Draw left and right borders
    for (int i = 0; i < height; i++) {
        vga_plot_pixel(x, y + i, color);           // Left border
        vga_plot_pixel(x + width - 1, y + i, color); // Right border
    }
}
void draw_happy_face(int x, int y) {
	// eye
	vga_plot_pixel(x,y,VGA_BLUE);
	// eye
	vga_plot_pixel(x+10,y,VGA_BLUE);
	// mouth
	vga_plot_pixel(x,	y+8,VGA_BLUE);
	vga_plot_pixel(x+1,	y+9,VGA_BLUE);
	vga_plot_pixel(x+2,	y+10,VGA_BLUE);
	vga_plot_pixel(x+3,	y+10,VGA_BLUE);
	vga_plot_pixel(x+4,	y+10,VGA_BLUE);
	vga_plot_pixel(x+5,	y+10,VGA_BLUE);
	vga_plot_pixel(x+6,	y+10,VGA_BLUE);
	vga_plot_pixel(x+7,	y+10,VGA_BLUE);
	vga_plot_pixel(x+8,	y+10,VGA_BLUE);
	vga_plot_pixel(x+9,	y+9,VGA_BLUE);
	vga_plot_pixel(x+10,y+8,VGA_BLUE);
}

void test_palette() {
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 16; j++) {
            draw_rectangle(j * 20, i * 12, 20, 12, i * 16 + j);
        }
    }
}

#define GRID_X_OFFSET 16    // X-coordinate offset for the grid
#define GRID_Y_OFFSET 16    // Y-coordinate offset for the grid
#define ICON_WIDTH 32       // Width of each icon
#define ICON_HEIGHT 20      // Height of each icon
#define GRID_SPACING 40     // Vertical space between icons
#define MAX_ICONS_PER_COLUMN 3  // Limit to 3 icons per column
#define TEXT_OFFSET 9       // Text offset per character
// Draw the desktop icons using the grid system

void window_manager () { 

    
}
void draw_desktop() {
    draw_filled_rectangle(0, 0, 320, 180, VGA_ORANGE);  // Main desktop background

    draw_filled_rectangle(0, 180, 320, 20, BLACK);      // Taskbar

    // Icon positions - the grid system
    int x = GRID_X_OFFSET;
    int y = GRID_Y_OFFSET;

    // Draw the Files Icon
    draw_filled_rectangle(x, y, ICON_WIDTH, ICON_HEIGHT, YELLOW); // folder body
    draw_rectangle(x, y, ICON_WIDTH, ICON_HEIGHT, BLACK);
    draw_filled_rectangle(x, y-4, 20, 5, YELLOW); // folder flap
    draw_rectangle(x, y-4, 20, 5, BLACK);
    vga_draw_string(x-4, y + 24, "files", VGA_WHITE);  

    // Task Manager Icon
    y += GRID_SPACING;  // Move down vertically by the defined spacing
    draw_filled_rectangle(x, y, ICON_WIDTH, ICON_HEIGHT, WHITE); // task manager body
    draw_rectangle(x, y, ICON_WIDTH, ICON_HEIGHT, BLACK);
    draw_filled_rectangle(x+1, y+1, 5, 5, GREEN); // Close button 
    draw_filled_rectangle(x + 7, y+1, 5, 5, YELLOW); // Minimize button
    draw_filled_rectangle(x + 13, y+1, 5, 5, RED); // Maximize button
    vga_draw_string(x-4, y + 24, "tasks", VGA_WHITE);  

    // Settings Icon
    y += GRID_SPACING;  // Move down vertically by the defined spacing
    draw_filled_rectangle(x, y, ICON_WIDTH, ICON_HEIGHT, DARK_GRAY); // settings body
    draw_rectangle(x, y, ICON_WIDTH, ICON_HEIGHT, BLACK);
    vga_draw_string(x + 12, y + 4, "*", VGA_BLACK); 
    vga_draw_string(x-4, y + 24, "settings", VGA_WHITE);  

    // Text Editor Icon (moved to next column)
    x += ICON_WIDTH + GRID_SPACING;  // Move to the next column (right)
    y = GRID_Y_OFFSET;  // Reset y to the top of the new column

    draw_filled_rectangle(x, y, ICON_WIDTH, ICON_HEIGHT, WHITE); // text editor body

    draw_rectangle(x, y, ICON_WIDTH, ICON_HEIGHT, BLACK);
    vga_draw_string(x + 12, y + 4, "T", VGA_BLACK); // T for text (adjusted y)
    vga_draw_string(x-4, y + 24, "edit", VGA_WHITE);  

    // Terminal Icon
    y += GRID_SPACING;  // Move down vertically by the defined spacing
    draw_filled_rectangle(x, y, ICON_WIDTH, ICON_HEIGHT, BLACK); // terminal body
    draw_rectangle(x, y, ICON_WIDTH, ICON_HEIGHT, BLACK);
    vga_draw_string(x + 6, y + 4, ">", VGA_WHITE); // Command prompt symbol (adjusted y)
    vga_draw_string(x-4, y + 24, "term", VGA_WHITE);  

    //vga_draw_small_string(200, 200, "test", VGA_WHITE);
}
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200  // Total pixel height of the screen
#define LINE_HEIGHT 13
#define CHARACTER_WIDTH 8 
#define MAX_LINES (SCREEN_HEIGHT / LINE_HEIGHT)
#define MAX_CHARACTERS_PER_LINE (SCREEN_WIDTH / CHARACTER_WIDTH)
// Screen buffer for storing text and colors
static int console_y = 0;  // The current Y position (in terms of lines)
static int console_x = 0;  // The current X position (in characters)
static char screen_buffer[SCREEN_HEIGHT][MAX_CHARACTERS_PER_LINE];  // Text buffer
static unsigned short color_buffer[SCREEN_HEIGHT][MAX_CHARACTERS_PER_LINE];  // Color buffer


void vga_init() {
    write_regs(g_320x200x256);
    set_palette();
    
}

void vga_desktop() {
    draw_desktop();
    return;
    
}

void draw_box(uint16_t x_start, uint16_t y_start, uint16_t width, uint16_t height, 
              const char* content, uint8_t border_color, uint8_t text_color) {
    // ASCII box-drawing characters
    char top_left = '\xDA';     // ╔
    char top_right = '\xBF';    // ╗
    char bottom_left = '\xC0';  // ╚
    char bottom_right = '\xD9'; // ╝
    char horizontal = '\xC4';   // ═
    char vertical = '\xB3';     // ║

    // Clear the area where the box will be drawn
    for (uint16_t y = 0; y < height; y++) {
        for (uint16_t x = x_start; x < x_start + width; x++) {
            vga_place_char(x, y_start + y, ' ', border_color);
        }
    }

    // Draw top border
    vga_place_char(x_start, y_start, top_left, border_color);
    for (uint16_t i = 1; i < width - 1; i++) {
        vga_place_char(x_start + i, y_start, horizontal, border_color);
    }
    vga_place_char(x_start + width - 1, y_start, top_right, border_color);

    // Draw sides and content
    for (uint16_t y = 1; y < height - 1; y++) {
        vga_place_char(x_start, y_start + y, vertical, border_color);
        if (y == 1 && content) {
            for (size_t i = 0; i < flopstrlen(content) && i < width - 2; i++) {
                vga_place_char(x_start + 1 + i, y_start + y, content[i], text_color);
            }
        }
        vga_place_char(x_start + width - 1, y_start + y, vertical, border_color);
    }

    // Draw bottom border
    vga_place_char(x_start, y_start + height - 1, bottom_left, border_color);
    for (uint16_t i = 1; i < width - 1; i++) {
        vga_place_char(x_start + i, y_start + height - 1, horizontal, border_color);
    }
    vga_place_char(x_start + width - 1, y_start + height - 1, bottom_right, border_color);
}