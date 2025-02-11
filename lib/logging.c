#include <stdint.h>
#include <stddef.h>
#include "../apps/echo.h"
#include "../drivers/vga/vgahandler.h"


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

void log_step(const char *message, uint8_t color) {
    echo("-> ", LIGHT_GRAY); // Arrow prefix
    echo(message, color);   // Custom message and color
}