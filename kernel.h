#ifndef KERNEL_H
#define KERNEL_H

#include "multiboot/multiboot.h"
#include <stdint.h>

int kmain(uint32_t magic, multiboot_info_t *mb_info);



void panic(uint32_t address, const char* msg, const char* err);
#define PANIC(msg, err) panic(__builtin_return_address(0), msg, err)

// Common panic macros
#define PANIC_OUT_OF_MEMORY() PANIC("Out of memory error", "OUT_OF_MEMORY")
#define PANIC_MULTIBOOT_MAGIC_NOT_FOUND() PANIC("Multiboot magic number not found", "MULTIBOOT_MAGIC_NOT_FOUND")
#define PANIC_MULTIBOOT_INFO_NOT_FOUND() PANIC("Multiboot info structure not found", "MULTIBOOT_INFO_NOT_FOUND")
#define PANIC_MEMORY_MAP_NOT_FOUND() PANIC("Memory map not found", "MEMORY_MAP_NOT_FOUND")
#define PANIC_FILESYSTEM_CREATION() PANIC("Failed to create filesystem", "FILESYSTEM_CREATION")
#define PANIC_KERNEL_NOT_FOUND() PANIC("Kernel not found", "KERNEL_NOT_FOUND")

#endif // KERNEL_H