#include "floptxt.h"
#include "../../drivers/vga/vgahandler.h"
#include "../../drivers/keyboard/keyboard.h"
#include "../../fs/tmpflopfs/tmpflopfs.h"
#include "../../fs/tmpflopfs/fileutils.h"
#include "../../apps/echo.h"
#include "../../drivers/time/floptime.h"
#include "../../task/sched.h"
#include "../../lib/str.h"
extern int extended;
static unsigned int scroll_offset = 0; // Number of lines scrolled from the top
static char text_buffer[MAX_TEXT_BUFFER_SIZE];
static char undo_buffer[MAX_TEXT_BUFFER_SIZE];
static char redo_buffer[MAX_TEXT_BUFFER_SIZE];
static char screen_buffer[MAX_TEXT_BUFFER_SIZE];
static unsigned int cursor_x = 0;
static unsigned int cursor_y = 0;
static int file_saved = 1;
static struct TmpFileSystem tmp_fs;

static int confirm_exit = 0;

void initialize_buffers() {
    for (int i = 0; i < MAX_TEXT_BUFFER_SIZE; i++) {
        text_buffer[i] = '\0';
        undo_buffer[i] = '\0';
        redo_buffer[i] = '\0';
        screen_buffer[i] = '\0';
    }
}

void draw_floptxt_border() {
    uint16_t width = 80;  // Width of the VGA text interface
    uint16_t height = 25; // Height of the VGA text interface
    uint8_t border_color = WHITE;
    uint8_t background_color = BLACK;

    // ASCII box-drawing characters
    char top_left = '\xDA';  // ╔
    char top_right = '\xBF'; // ╗
    char bottom_left = '\xC0'; // ╚
    char bottom_right = '\xD9'; // ╝
    char horizontal = '\xC4'; // ═
    char vertical = '\xB3'; // ║
    char junction = '\xC5'; // ╬

    // Clear screen and draw outer border
    for (uint16_t y = 0; y < height; y++) {
        for (uint16_t x = 0; x < width; x++) {
            vga_place_char(x, y, ' ', background_color);
        }
    }

    // Draw top border with title bar
    vga_place_char(0, 0, top_left, border_color);
    for (uint16_t i = 1; i < width - 1; i++) {
        vga_place_char(i, 0, horizontal, border_color);
    }
    vga_place_char(width - 1, 0, top_right, border_color);

    // Title in the top border
    const char *title = "floptxt 1.0 - Ctrl+S: Save | Ctrl+C: Exit ";
    for (uint16_t i = 0; title[i] != '\0'; i++) {
        vga_place_char((width / 2 - (flopstrlen(title) / 2)) + i, 0, title[i], border_color);
    }

    // Draw side borders
    for (uint16_t y = 1; y < height - 1; y++) {
        vga_place_char(0, y, vertical, border_color);
        vga_place_char(width - 1, y, vertical, border_color);
    }

    // Draw bottom border
    vga_place_char(0, height - 1, bottom_left, border_color);
    for (uint16_t i = 1; i < width - 1; i++) {
        vga_place_char(i, height - 1, horizontal, border_color);
    }
    vga_place_char(width - 1, height - 1, bottom_right, border_color);

    // Instructions in the bottom border
    const char *instructions = " [F1: Help | F2: Save As] ";
    for (uint16_t i = 0; instructions[i] != '\0'; i++) {
        vga_place_char((width / 2 - (flopstrlen(instructions) / 2)) + i, height - 1, instructions[i], border_color);
    }

    // Draw an inner box for content
    uint16_t inner_top = 3;
    uint16_t inner_bottom = height - 4;
    uint16_t inner_left = 2;
    uint16_t inner_right = width - 3;

    // Top of inner box
    vga_place_char(inner_left, inner_top, top_left, border_color);
    for (uint16_t i = inner_left + 1; i < inner_right; i++) {
        vga_place_char(i, inner_top, horizontal, border_color);
    }
    vga_place_char(inner_right, inner_top, top_right, border_color);

    // Sides of inner box
    for (uint16_t y = inner_top + 1; y < inner_bottom; y++) {
        vga_place_char(inner_left, y, vertical, border_color);
        vga_place_char(inner_right, y, vertical, border_color);
    }

    // Bottom of inner box
    vga_place_char(inner_left, inner_bottom, bottom_left, border_color);
    for (uint16_t i = inner_left + 1; i < inner_right; i++) {
        vga_place_char(i, inner_bottom, horizontal, border_color);
    }
    vga_place_char(inner_right, inner_bottom, bottom_right, border_color);

    // Draw horizontal dividing line inside the inner box
    uint16_t divider_y = (inner_top + inner_bottom) / 2;
    for (uint16_t i = inner_left + 1; i < inner_right; i++) {
        vga_place_char(i, divider_y, horizontal, border_color);
    }
    vga_place_char(inner_left, divider_y, junction, border_color);
    vga_place_char(inner_right, divider_y, junction, border_color);
}

void draw_text() {
    unsigned int index = scroll_offset * (VGA_WIDTH - 2);
    for (unsigned int y = 2; y < VGA_HEIGHT - 1; y++) {
        for (unsigned int x = 1; x < VGA_WIDTH - 1; x++) {
            if (index < MAX_TEXT_BUFFER_SIZE) {
                char c = text_buffer[index];
                if (c != screen_buffer[index]) {
                    vga_place_char(x, y, c ? c : ' ', 0x07);
                    screen_buffer[index] = c;
                }
                index++;
            } else {
                vga_place_char(x, y, ' ', 0x07);
            }
        }
    }
    vga_set_cursor_position(cursor_x + 1, cursor_y + 2 - scroll_offset);
}

void scroll_if_needed() {
    if (cursor_y - scroll_offset >= VGA_HEIGHT - 3) {
        scroll_offset++;
        draw_text();
    } else if (cursor_y < scroll_offset) {
        scroll_offset--;
        draw_text();
    }
}

void insert_char(char c) {
    if (cursor_x >= 0 && cursor_x < VGA_WIDTH - 2 && cursor_y >= 0 && cursor_y < MAX_TEXT_BUFFER_SIZE / (VGA_WIDTH - 2)) {
        unsigned int index = cursor_y * (VGA_WIDTH - 2) + cursor_x;
        if (index < MAX_TEXT_BUFFER_SIZE - 1) {
            text_buffer[index] = c;
            cursor_x++;
            if (cursor_x >= VGA_WIDTH - 2) {
                cursor_x = 0;
                cursor_y++;
                scroll_if_needed();
            }
        }
    }
}

void backspace() {
    if (cursor_x > 0 || cursor_y > 0) {
        if (cursor_x == 0) {
            cursor_y--;
            cursor_x = VGA_WIDTH - 3; // Account for border width
        } else {
            cursor_x--;
        }
        unsigned int index = cursor_y * (VGA_WIDTH - 2) + cursor_x;
        text_buffer[index] = '\0';

        // Redraw space at the cursor position
        vga_place_char(cursor_x + 1, cursor_y + 2 - scroll_offset, ' ', 0x07);
    }
}


void save_file(const char *filename) {
    TmpFileDescriptor *fd = flop_open(filename, FILE_MODE_WRITE);
    if (fd) {
        flop_write(fd, text_buffer, MAX_TEXT_BUFFER_SIZE);
        flop_close(fd);
        file_saved = 1;
        echo("File saved successfully.\n", GREEN);
    } else {
        echo("Error saving file.\n", RED);
    }
}

void process_input(char c, const char *filename) {
    
    if (confirm_exit) {
        if (c == 'y' || c == 'Y') {
            remove_current_task();
        } else if (c == 'n' || c == 'N') {
            confirm_exit = 0;
            draw_floptxt_border();
            draw_text();
        }
        return;
    }
    switch (c) {
        case ARROW_UP:
            if (cursor_y > 0) cursor_y--;
            scroll_if_needed();
            break;
        case ARROW_DOWN:
            if (cursor_y < MAX_TEXT_BUFFER_SIZE / (VGA_WIDTH - 2)) cursor_y++;
            scroll_if_needed();
            break;
        case ARROW_LEFT:
            if (cursor_x > 0) cursor_x--;
            else if (cursor_y > 0) {
                cursor_y--;
                cursor_x = VGA_WIDTH - 3;
            }
            scroll_if_needed();
            break;
        case ARROW_RIGHT:
            if (cursor_x < VGA_WIDTH - 3) cursor_x++;
            else {
                cursor_x = 0;
                cursor_y++;
            }
            scroll_if_needed();
            break;

    }
    if (c == '\b') {
        backspace();
    } else if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        scroll_if_needed();
    } else if (c >= 32 && c <= 126) {
        insert_char(c);
    } else if (c == 0x1F) { // Ctrl+S
        save_file(filename);
    } else if (c == 0x03) { // Ctrl+C
        confirm_exit = 1;
        echo("Are you sure you want to exit? (Y/N)\n", YELLOW);
    }
}

void floptxt_task(void *arg) {
    char c;
    unsigned char key;
    vga_clear_terminal();
    initialize_buffers();
    draw_floptxt_border();

    if (arg != NULL) {
        TmpFileDescriptor *fd = flop_open((const char *)arg, FILE_MODE_READ);
        if (fd) {
            flop_read(fd, text_buffer, MAX_TEXT_BUFFER_SIZE);
            flop_close(fd);
        }
    }

    while (1) {
        draw_text();
        c = try_get_char();
        TmpFileDescriptor *fd = flop_open((const char *)arg, FILE_MODE_READ);
        if (c) process_input(c, (const char *)arg);

    }
}
