#ifndef IO_H
#define IO_H

#include <stdint.h>
#include <stdbool.h>

// Lock and unlock functions to ensure thread-safe I/O operations
void lock_io();
void unlock_io();

// Byte (8-bit) I/O functions
uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t data);

// Word (16-bit) I/O functions
uint16_t inw(uint16_t port);
void outw(uint16_t port, uint16_t data);

// Long word (32-bit) I/O functions
uint32_t inl(uint16_t port);
void outl(uint16_t port, uint32_t data);

// String I/O operations
void insb(uint16_t port, uint8_t *buffer, uint32_t count);
void outsb(uint16_t port, const uint8_t *buffer, uint32_t count);
void insw(uint16_t port, uint16_t *buffer, uint32_t count);
void outsw(uint16_t port, const uint16_t *buffer, uint32_t count);
void insl(uint16_t port, uint32_t *buffer, uint32_t count);
void outsl(uint16_t port, const uint32_t *buffer, uint32_t count);

// Utility functions
void wait_for_io(); // Simple delay for timing

// Polling and control functions
bool check_io_ready(uint16_t status_port, uint8_t ready_mask);
uint8_t read_control_register(uint16_t control_port);
void write_control_register(uint16_t control_port, uint8_t value);
void reset_device(uint16_t reset_port);

// Memory copy functions for I/O ports
void io_memcpy_to_port(uint16_t port, const void *buffer, uint32_t size);
void io_memcpy_from_port(uint16_t port, void *buffer, uint32_t size);

// Bit manipulation functions
void toggle_io_bit(uint16_t control_port, uint8_t bit);
void set_io_bit(uint16_t control_port, uint8_t bit);
void clear_io_bit(uint16_t control_port, uint8_t bit);

#endif // IO_H

