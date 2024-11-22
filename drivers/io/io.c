#include <stdint.h>
#include <stdbool.h>

// A simple flag to track if I/O operations are in progress
volatile bool io_locked = false;

void lock_io() {
    while (__sync_lock_test_and_set(&io_locked, true)) {
        // Busy-wait until the lock is released
    }
}

void unlock_io() {
    __sync_lock_release(&io_locked);
}

// Read a byte from the specified port with locking
uint8_t inb(uint16_t port) {
    lock_io();  // Lock I/O operations
    uint8_t data;
    __asm__ volatile ("inb %1, %0" : "=a"(data) : "Nd"(port));
    unlock_io();  // Unlock I/O operations
    return data;
}

// Write a byte to the specified port with locking
void outb(uint16_t port, uint8_t data) {
    lock_io();  // Lock I/O operations
    __asm__ volatile ("outb %0, %1" : : "a"(data), "Nd"(port));
    unlock_io();  // Unlock I/O operations
}

// Read a word (2 bytes) from the specified port with locking
uint16_t inw(uint16_t port) {
    lock_io();  // Lock I/O operations
    uint16_t data;
    __asm__ volatile ("inw %1, %0" : "=a"(data) : "Nd"(port));
    unlock_io();  // Unlock I/O operations
    return data;
}

// Write a word (2 bytes) to the specified port with locking
void outw(uint16_t port, uint16_t data) {
    lock_io();  // Lock I/O operations
    __asm__ volatile ("outw %0, %1" : : "a"(data), "Nd"(port));
    unlock_io();  // Unlock I/O operations
}

// Read a long word (4 bytes) from the specified port with locking
uint32_t inl(uint16_t port) {
    lock_io();  // Lock I/O operations
    uint32_t data;
    __asm__ volatile ("inl %1, %0" : "=a"(data) : "Nd"(port));
    unlock_io();  // Unlock I/O operations
    return data;
}

// Write a long word (4 bytes) to the specified port with locking
void outl(uint16_t port, uint32_t data) {
    lock_io();  // Lock I/O operations
    __asm__ volatile ("outl %0, %1" : : "a"(data), "Nd"(port));
    unlock_io();  // Unlock I/O operations
}

// Read a string of bytes from the specified port with locking
void insb(uint16_t port, uint8_t *buffer, uint32_t count) {
    lock_io();  // Lock I/O operations
    __asm__ volatile (
        "rep insb"
        : "+D"(buffer), "+c"(count)
        : "d"(port)
        : "memory");
    unlock_io();  // Unlock I/O operations
}

// Write a string of bytes to the specified port with locking
void outsb(uint16_t port, const uint8_t *buffer, uint32_t count) {
    lock_io();  // Lock I/O operations
    __asm__ volatile (
        "rep outsb"
        : "+S"(buffer), "+c"(count)
        : "d"(port)
        : "memory");
    unlock_io();  // Unlock I/O operations
}

// Read a string of words (2 bytes each) from the specified port with locking
void insw(uint16_t port, uint16_t *buffer, uint32_t count) {
    lock_io();  // Lock I/O operations
    __asm__ volatile (
        "rep insw"
        : "+D"(buffer), "+c"(count)
        : "d"(port)
        : "memory");
    unlock_io();  // Unlock I/O operations
}

// Write a string of words (2 bytes each) to the specified port with locking
void outsw(uint16_t port, const uint16_t *buffer, uint32_t count) {
    lock_io();  // Lock I/O operations
    __asm__ volatile (
        "rep outsw"
        : "+S"(buffer), "+c"(count)
        : "d"(port)
        : "memory");
    unlock_io();  // Unlock I/O operations
}

// Read a string of long words (4 bytes each) from the specified port with locking
void insl(uint16_t port, uint32_t *buffer, uint32_t count) {
    lock_io();  // Lock I/O operations
    __asm__ volatile (
        "rep insl"
        : "+D"(buffer), "+c"(count)
        : "d"(port)
        : "memory");
    unlock_io();  // Unlock I/O operations
}

// Write a string of long words (4 bytes each) to the specified port with locking
void outsl(uint16_t port, const uint32_t *buffer, uint32_t count) {
    lock_io();  // Lock I/O operations
    __asm__ volatile (
        "rep outsl"
        : "+S"(buffer), "+c"(count)
        : "d"(port)
        : "memory");
    unlock_io();  // Unlock I/O operations
}
