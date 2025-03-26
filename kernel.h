#ifndef KERNEL_H
#define KERNEL_H

#include "multiboot/multiboot.h"
#include <stdint.h>

int kmain(uint32_t magic, multiboot_info_t *mb_info);



void panic(uint32_t address, const char* msg, const char* err);
#define PANIC(addr, msg, err) panic(addr, msg, err)

// Common panic macros
#define PANIC_OUT_OF_MEMORY(addr) PANIC(addr,"Out of memory error", "OUT_OF_MEMORY")
#define PANIC_MULTIBOOT_MAGIC_NOT_FOUND(addr) PANIC(addr,"Multiboot magic number not found", "MULTIBOOT_MAGIC_NOT_FOUND")
#define PANIC_MULTIBOOT_INFO_NOT_FOUND(addr) PANIC(addr,"Multiboot info structure not found", "MULTIBOOT_INFO_NOT_FOUND")
#define PANIC_MEMORY_MAP_NOT_FOUND(addr) PANIC(addr,"Memory map not found", "MEMORY_MAP_NOT_FOUND")
#define PANIC_FILESYSTEM_CREATION(addr) PANIC(addr,"Failed to create filesystem", "FILESYSTEM_CREATION")
#define PANIC_KERNEL_NOT_FOUND(addr) PANIC(addr,"Kernel not found", "KERNEL_NOT_FOUND")
#define PANIC_FAILED_TO_ALLOCATE_MEMORY(addr) PANIC(addr,"Failed to allocate memory", "FAILED_TO_ALLOCATE_MEMORY")
#define PANIC_FAILED_TO_FREE_MEMORY(addr) PANIC(addr,"Failed to free memory", "FAILED_TO_FREE_MEMORY")
#define PANIC_INVALID_SLAB_CACHE(addr) PANIC(addr,"Invalid slab cache", "INVALID_SLAB_CACHE")
#define PANIC_SLAB_CACHE_NOT_FOUND(addr) PANIC(addr,"Slab cache not found", "SLAB_CACHE_NOT_FOUND")
#define PANIC_FAILED_SLAB_ALLOCATION(addr) PANIC(addr,"Failed to allocate slab", "INVALID_SLAB_ALLOCATION")
#define PANIC_FAILED_SLAB_FREE(addr) PANIC(addr,"Failed to free slab", "INVALID_SLAB_FREE")
#define PANIC_FAILED_TO_CREATE_SLAB(addr) PANIC(addr,"Failed to create slab", "FAILED_TO_CREATE_SLAB")
#define PANIC_FAILED_TO_FIND_CACHE_CONTAINING_SLAB(addr) PANIC(addr,"Failed to find cache containing slab", "FAILED_TO_FIND_CACHE_CONTAINING_SLAB")
#endif // KERNEL_H