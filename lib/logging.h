#ifndef LOGGING_H
#define LOGGING_H

#include <stdint.h>
#include <stddef.h>
void log_address(const char *msg, uintptr_t addr) ;
void log_uint(const char* prefix, uint32_t value) ;
void log_step(const char *message, uint8_t color) ;
void log_error(const char *message, uint8_t color) ;
#endif