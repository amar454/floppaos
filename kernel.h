#ifndef KERNEL_H
#define KERNEL_H

#include "multiboot/multiboot.h"
#include <stdint.h>

int kmain(uint32_t magic, multiboot_info_t *mb_info);


void warn(uint32_t address, const char* msg, const char* warning);

// kernel warning macros
#define WARN(addr, msg, err) warn(addr, msg, warning)
#define WARN_LOW_MEMORY_AVAILABLE(addr, used_mem)) WARN(used_mem_addr_end), "low amount of memory remaining: less than 32mb. Try clean_mem in the terminal or consider cancelling processes that are taking up large amounts of ram, or background processes.", "LOW_MEMORY_AVAILABLE")



void warn(uint32_t address, const char* msg, const char* warning);

// kernel warning macros
#define WARN(addr, msg, err) warn(addr, msg, warning)
#define WARN_LOW_MEMORY_AVAILABLE(addr, used_mem) WARN(used_mem_addr_end), "low amount of memory remaining: less than 32mb. Try clean_mem in the terminal or consider cancelling processes that are taking up large amounts of ram, or background processes.", "LOW_MEMORY_AVAILABLE")




void panic(uint32_t address, const char* msg, const char* err);
#define PANIC(addr, msg, err) panic(addr, msg, err)
// Common panic macros
#define PANIC_GENERIC(addr) PANIC(addr,"Generic panic error", "GENERIC_PANIC")
#define PANIC_OUT_OF_MEMORY(addr) PANIC(addr,"Out of memory error", "OUT_OF_MEMORY")
#define PANIC_MULTIBOOT_MAGIC_NOT_FOUND(addr) PANIC(addr,"Multiboot magic number not found", "MULTIBOOT_MAGIC_NOT_FOUND")
#define PANIC_MULTIBOOT_INFO_NOT_FOUND(addr) PANIC(addr,"Multiboot info structure not found", "MULTIBOOT_INFO_NOT_FOUND")
#define PANIC_MEMORY_MAP_NOT_FOUND(addr) PANIC(addr,"Multiboot memory map not found", "MEMORY_MAP_NOT_FOUND")
#define PANIC_NOT_ENOUGH_MEMORY_AVAILABLE(mmap_end_addr) PANIC(mmap_end_addr, "Too little available memory to run floppaOS. (less than 16 mb)", "NOT_ENOUGH_MEMORY_AVAILABLE")
#define PANIC_TMPFS_CREATION_FAILED(addr) PANIC(addr,"tmpfs: Failed to create tmpfs", "TMP_CREATION_FAILED")
#define PANIC_PAGE_FAULT(page_addr) PANIC(addr, "Page Fault, rip", "PAGE_FAULT")
#define PANIC_PAGE_ALLOCATION_FAILED(last_allocated_page_addr) PANIC(last_allocated_page_addr, "Page allocation failed, not enough memory to allocate page, ", "PAGE_ALLOCATION_FAILED")
#define PANIC_FAILED_TO_ALLOCATE_MEMORY(addr) PANIC(addr,"Failed to allocate memory at ", "FAILED_TO_ALLOCATE_MEMORY")
#define PANIC_MEMORY_MAP_NOT_FOUND(addr) PANIC(addr,"Multiboot memory map not found", "MEMORY_MAP_NOT_FOUND")
#define PANIC_NOT_ENOUGH_MEMORY_AVAILABLE(mmap_end_addr) PANIC(mmap_end_addr, "Too little available memory to run floppaOS. (less than 16 mb)", "NOT_ENOUGH_MEMORY_AVAILABLE")
#define PANIC_TMPFS_CREATION_FAILED(addr) PANIC(addr,"tmpfs: Failed to create tmpfs", "TMP_CREATION_FAILED")
#define PANIC_PAGE_FAULT(page_addr) PANIC(addr, "Page Fault, rip", "PAGE_FAULT")
#define PANIC_PAGE_ALLOCATION_FAILED(last_allocated_page_addr) PANIC(last_allocated_page_addr, "Page allocation failed, not enough memory to allocate page, ", "PAGE_ALLOCATION_FAILED")
#define PANIC_FAILED_TO_ALLOCATE_MEMORY(addr) PANIC(addr,"Failed to allocate memory at ", "FAILED_TO_ALLOCATE_MEMORY")
#define PANIC_FAILED_TO_FREE_MEMORY(addr) PANIC(addr,"Failed to free memory", "FAILED_TO_FREE_MEMORY")
#define PANIC_INVALID_SLAB_CACHE(addr) PANIC(addr,"Invalid slab cache", "INVALID_SLAB_CACHE")
#define PANIC_SLAB_CACHE_NOT_FOUND(addr) PANIC(addr,"Slab cache not found", "SLAB_CACHE_NOT_FOUND")
#define PANIC_FAILED_SLAB_ALLOCATION(addr) PANIC(addr,"Failed to allocate slab", "INVALID_SLAB_ALLOCATION")
#define PANIC_FAILED_SLAB_FREE(addr) PANIC(addr,"Failed to free slab", "INVALID_SLAB_FREE")
#define PANIC_FAILED_TO_CREATE_SLAB(addr) PANIC(addr,"Failed to create slab", "FAILED_TO_CREATE_SLAB")
#define PANIC_PMM_NOT_INITIALIZED(addr) PANIC(addr,"PMM not initialized", "PMM_NOT_INITIALIZED")
#define PANIC_HEAP_CORRUPTION(addr) PANIC(addr, "Heap corrupted", "PMM_HEAP_CORRUPTION")
#define PANIC_FAILED_TO_FIND_CACHE_CONTAINING_SLAB(addr) PANIC(addr,"Failed to find cache containing slab", "FAILED_TO_FIND_CACHE_CONTAINING_SLAB")
#endif // KERNEL_H
