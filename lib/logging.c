#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include "../apps/echo.h"
#include "../drivers/vga/vgahandler.h"
#include "str.h"

// local function for flopitoa for printing unsigned integers and memory addresses.
char* memflopitoa(uint32_t value, char* buffer, uint32_t base) {
    if (base < 2 || base > 16) {
        buffer[0] = '\0';
        return buffer;
    }

    char* digits = "0123456789ABCDEF";
    char* ptr = buffer;
    char* end = buffer;

    // Handle zero value
    if (value == 0) {
        *end++ = '0';
        *end = '\0';
        return buffer;
    }

    // Convert number to string
    while (value) {
        *end++ = digits[value % base];
        value /= base;
    }

    *end = '\0';

    // Reverse the string
    for (--end; ptr < end; ++ptr, --end) {
        char tmp = *ptr;
        *ptr = *end;
        *end = tmp;
    }

    return buffer;
}
void log_address(const char* prefix, uint32_t address) {
    char buffer[16];
    memflopitoa(address, buffer, 16);
    echo(prefix, LIGHT_GRAY);
    echo("0x", CYAN);
    echo(buffer, CYAN);
    echo("\n", LIGHT_GRAY);
}

void log_uint(const char* prefix, uint32_t value) {
    char buffer[16];
    memflopitoa(value, buffer, 10);
    echo(prefix, LIGHT_GRAY);
    echo(buffer, CYAN);
    echo("\n", LIGHT_GRAY);
}

// kernel logging function, with a custom message 
void log(const char *message, uint8_t color) {
    echo("-> ", LIGHT_GRAY); // Arrow prefix
    echo(message, color);   // Custom message and color
}

void log_error(const char *message, uint8_t color) {
    echo("-> ", LIGHT_GRAY); // Arrow prefix
    echo("floppaOS has run into an error.\n", RED);   // Custom message and color
    echo("-> Error description: ", LIGHT_GRAY); // Arrow prefix
    echo(message, color);   // Custom message and color
}
#define LOG_BUF_SIZE 256

void log_f(const char* fmt, ...) {
    char buf[LOG_BUF_SIZE];
    va_list args;
    va_start(args, fmt);
    flopsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    log(buf, LIGHT_GRAY);
}
