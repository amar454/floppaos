#ifndef LOGGING_H
#define LOGGING_H

#include <stdint.h>
#include <stddef.h>
#include "../drivers/vga/vgahandler.h"
void log_address(const char *msg, uint32_t address) ;
char* memflopitoa(uint32_t value, char* buffer, uint32_t base) ;
void log_uint(const char* prefix, uint32_t value) ;
void log(const char *message, uint8_t color) ;
void log_error(const char *message, uint8_t color) ;
void log_f(const char *fmt, ...);
#endif