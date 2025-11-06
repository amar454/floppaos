

#include "floptime.h"
#include "../../lib/str.h"
#include <stdbool.h>
#include <stddef.h>
#include "../../apps/echo.h"
#include "../vga/vgahandler.h"
#include "../../task/sched.h"

char current_time_string[32];

static unsigned char read_cmos(unsigned char reg) {
    unsigned char val;

    __asm__ volatile("outb %1, %0" : : "dN"((unsigned short) 0x70), "a"(reg));

    __asm__ volatile("inb %1, %0" : "=a"(val) : "dN"((unsigned short) 0x71));

    return val;
}

static int bcd_to_binary(int val) {
    return ((val / 16) * 10) + (val & 0x0F);
}

void time_get_current(struct Time* time) {
    if (time != NULL) {
        while (read_cmos(0x0A) & 0x80)
            ;
        time->second = bcd_to_binary(read_cmos(0x00));
        time->minute = bcd_to_binary(read_cmos(0x02));
        time->hour = bcd_to_binary(read_cmos(0x04));
        time->day = bcd_to_binary(read_cmos(0x07));
        time->month = bcd_to_binary(read_cmos(0x08));
        time->year = bcd_to_binary(read_cmos(0x09)) + 2000;
    }
}

void time_to_string(struct Time* time, char* buffer, size_t size) {
    if (time) {
        flopsnprintf(buffer,
                     size,
                     "%02d-%02d-%04d %02d:%02d:%02d",
                     time->day,
                     time->month,
                     time->year,
                     time->hour,
                     time->minute,
                     time->second);
    } else {
        flopsnprintf(buffer, size, "Invalid time");
    }
}

static char last_time_string[20] = {0};
static bool time_box_exists = false;
static uint16_t last_x_start = VGA_WIDTH;
static uint16_t last_time_length = 0;

void display_time_top_right(const char* current_time_string) {
    uint16_t time_length = flopstrlen(current_time_string);
    uint16_t box_width = time_length + 4;
    uint16_t x_start = VGA_WIDTH - box_width;
    uint16_t y_position = 0;
    uint8_t border_color = LIGHT_BLUE;
    uint8_t time_color = WHITE;

    char top_left = '\xDA';
    char top_right = '\xBF';
    char bottom_left = '\xC0';
    char bottom_right = '\xD9';
    char horizontal = '\xC4';
    char vertical = '\xB3';

    for (uint16_t y = 0; y < 3; y++) {
        for (uint16_t x = x_start; x < VGA_WIDTH; x++) {
            vga_place_char(x, y_position + y, ' ', border_color);
        }
    }

    vga_place_char(x_start, y_position, top_left, border_color);
    for (uint16_t i = 0; i < time_length + 2; i++) {
        vga_place_char(x_start + 1 + i, y_position, horizontal, border_color);
    }
    vga_place_char(x_start + time_length + 3, y_position, top_right, border_color);

    vga_place_char(x_start, y_position + 1, vertical, border_color);
    for (size_t i = 0; i < time_length; i++) {
        vga_place_char(x_start + 2 + i, y_position + 1, current_time_string[i], time_color);
    }
    vga_place_char(x_start + time_length + 3, y_position + 1, vertical, border_color);

    vga_place_char(x_start, y_position + 2, bottom_left, border_color);
    for (uint16_t i = 0; i < time_length + 2; i++) {
        vga_place_char(x_start + 1 + i, y_position + 2, horizontal, border_color);
    }
    vga_place_char(x_start + time_length + 3, y_position + 2, bottom_right, border_color);

    flopstrcopy(last_time_string, current_time_string, sizeof(last_time_string));
    last_x_start = x_start;
    last_time_length = time_length;
}

void sleep_seconds(int seconds) {
    struct Time start_time, current_time;
    int elapsed_seconds = 0;

    time_get_current(&start_time);

    while (elapsed_seconds < seconds) {
        time_get_current(&current_time);

        elapsed_seconds = current_time.second - start_time.second;
        if (elapsed_seconds < 0) {
            elapsed_seconds += 60;
        }
    }
}

void time_task(void* arg) {
    struct Time* time = (struct Time*) arg;
    time_get_current(time);

    flopsnprintf(current_time_string,
                 sizeof(current_time_string),
                 "%02d-%02d-%04d %02d:%02d:%02d",
                 time->day,
                 time->month,
                 time->year,
                 time->hour,
                 time->minute,
                 time->second);
    display_time_top_right(current_time_string);
}