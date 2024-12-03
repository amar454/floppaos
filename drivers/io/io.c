/*
Copyright 2024 Amar Djulovic <aaamargml@gmail.com>

This file is part of FloppaOS.

FloppaOS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

FloppaOS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with FloppaOS. If not, see <https://www.gnu.org/licenses/>.

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
