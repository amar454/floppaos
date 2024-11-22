/*
Copyright 2024 Amar Djulovic <aaamargml@gmail.com>
---------------------------------------------------------------
io.c 

I/O functions for FloppaOS, using inline assembly.

Includes locking and unlocking feature for safe multithreading.
---------------------------------------------------------------
*/

#include <stdint.h>
#include <stdbool.h>

// A simple flag to track if I/O operations are in progress
volatile bool io_locked = false;

// Lock I/O operations
void lock_io() {
    while (__sync_lock_test_and_set(&io_locked, true)) {
        // Busy-wait until the lock is released
    }
}

// Unlock I/O operations
void unlock_io() {
    __sync_lock_release(&io_locked);
}

// Read a byte from the specified port with locking
uint8_t inb(uint16_t port) {
    lock_io();
    uint8_t data;
    __asm__ volatile ("inb %1, %0" : "=a"(data) : "Nd"(port));
    unlock_io();
    return data;
}

// Write a byte to the specified port with locking
void outb(uint16_t port, uint8_t data) {
    lock_io();
    __asm__ volatile ("outb %0, %1" : : "a"(data), "Nd"(port));
    unlock_io();
}

// Read a word (2 bytes) from the specified port with locking
uint16_t inw(uint16_t port) {
    lock_io();
    uint16_t data;
    __asm__ volatile ("inw %1, %0" : "=a"(data) : "Nd"(port));
    unlock_io();
    return data;
}

// Write a word (2 bytes) to the specified port with locking
void outw(uint16_t port, uint16_t data) {
    lock_io();
    __asm__ volatile ("outw %0, %1" : : "a"(data), "Nd"(port));
    unlock_io();
}

// Read a long word (4 bytes) from the specified port with locking
uint32_t inl(uint16_t port) {
    lock_io();
    uint32_t data;
    __asm__ volatile ("inl %1, %0" : "=a"(data) : "Nd"(port));
    unlock_io();
    return data;
}

// Write a long word (4 bytes) to the specified port with locking
void outl(uint16_t port, uint32_t data) {
    lock_io();
    __asm__ volatile ("outl %0, %1" : : "a"(data), "Nd"(port));
    unlock_io();
}

// Read a string of bytes from the specified port with locking
void insb(uint16_t port, uint8_t *buffer, uint32_t count) {
    lock_io();
    __asm__ volatile (
        "rep insb"
        : "+D"(buffer), "+c"(count)
        : "d"(port)
        : "memory");
    unlock_io();
}

// Write a string of bytes to the specified port with locking
void outsb(uint16_t port, const uint8_t *buffer, uint32_t count) {
    lock_io();
    __asm__ volatile (
        "rep outsb"
        : "+S"(buffer), "+c"(count)
        : "d"(port)
        : "memory");
    unlock_io();
}

// Read a string of words (2 bytes each) from the specified port with locking
void insw(uint16_t port, uint16_t *buffer, uint32_t count) {
    lock_io();
    __asm__ volatile (
        "rep insw"
        : "+D"(buffer), "+c"(count)
        : "d"(port)
        : "memory");
    unlock_io();
}

// Write a string of words (2 bytes each) to the specified port with locking
void outsw(uint16_t port, const uint16_t *buffer, uint32_t count) {
    lock_io();
    __asm__ volatile (
        "rep outsw"
        : "+S"(buffer), "+c"(count)
        : "d"(port)
        : "memory");
    unlock_io();
}

// Read a string of long words (4 bytes each) from the specified port with locking
void insl(uint16_t port, uint32_t *buffer, uint32_t count) {
    lock_io();
    __asm__ volatile (
        "rep insl"
        : "+D"(buffer), "+c"(count)
        : "d"(port)
        : "memory");
    unlock_io();
}

// Write a string of long words (4 bytes each) to the specified port with locking
void outsl(uint16_t port, const uint32_t *buffer, uint32_t count) {
    lock_io();
    __asm__ volatile (
        "rep outsl"
        : "+S"(buffer), "+c"(count)
        : "d"(port)
        : "memory");
    unlock_io();
}

// Wait for I/O (simple delay for timing)
void wait_for_io() {
    __asm__ volatile ("outb %%al, $0x80" : : "a"(0));
}

// Check if a device is ready (polling a status register)
bool check_io_ready(uint16_t status_port, uint8_t ready_mask) {
    lock_io();
    uint8_t status;
    __asm__ volatile ("inb %1, %0" : "=a"(status) : "Nd"(status_port));
    unlock_io();
    return (status & ready_mask) != 0;
}

// Read from a control register
uint8_t read_control_register(uint16_t control_port) {
    lock_io();
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(control_port));
    unlock_io();
    return value;
}

// Write to a control register
void write_control_register(uint16_t control_port, uint8_t value) {
    lock_io();
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(control_port));
    unlock_io();
}

// Reset a device by sending a reset signal
void reset_device(uint16_t reset_port) {
    lock_io();
    __asm__ volatile ("outb %0, %1" : : "a"(0x00), "Nd"(reset_port));
    unlock_io();
}

// Copy a buffer to an I/O port
void io_memcpy_to_port(uint16_t port, const void *buffer, uint32_t size) {
    lock_io();
    __asm__ volatile (
        "rep outsb"
        : "+S"(buffer), "+c"(size)
        : "d"(port)
        : "memory");
    unlock_io();
}

// Copy data from an I/O port to a buffer
void io_memcpy_from_port(uint16_t port, void *buffer, uint32_t size) {
    lock_io();
    __asm__ volatile (
        "rep insb"
        : "+D"(buffer), "+c"(size)
        : "d"(port)
        : "memory");
    unlock_io();
}

// Toggle a bit in a control register
void toggle_io_bit(uint16_t control_port, uint8_t bit) {
    lock_io();
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(control_port));
    value ^= (1 << bit);
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(control_port));
    unlock_io();
}

// Set a specific bit in a control register
void set_io_bit(uint16_t control_port, uint8_t bit) {
    lock_io();
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(control_port));
    value |= (1 << bit);
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(control_port));
    unlock_io();
}

// Clear a specific bit in a control register
void clear_io_bit(uint16_t control_port, uint8_t bit) {
    lock_io();
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(control_port));
    value &= ~(1 << bit);
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(control_port));
    unlock_io();
}
