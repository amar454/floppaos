#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>

// Multiboot header magic number
#define MULTIBOOT_MAGIC 0x1BADB002

// Multiboot flags
#define MULTIBOOT_INFO_MEMORY 0x00000001
#define MULTIBOOT_INFO_BOOTDEV 0x00000002
#define MULTIBOOT_INFO_CMDLINE 0x00000004
#define MULTIBOOT_INFO_MODS 0x00000008
#define MULTIBOOT_INFO_AOUT_SYMS 0x00000010
#define MULTIBOOT_INFO_ELF_SHDR 0x00000020
#define MULTIBOOT_INFO_MEM_MAP 0x00000040
#define MULTIBOOT_INFO_DRIVE_INFO 0x00000080
#define MULTIBOOT_INFO_CONFIG_TABLE 0x00000100
#define MULTIBOOT_INFO_BOOT_LOADER 0x00000200
#define MULTIBOOT_INFO_APM_TABLE 0x00000400
#define MULTIBOOT_INFO_FRAMEBUFFER 0x00000800

// Multiboot info structure for 32-bit mode
typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    union {
        struct {
            uint32_t tabsize;
            uint32_t strsize;
            uint32_t addr;
        } aout_sym;
        struct {
            uint32_t num;
            uint32_t size;
            uint32_t addr;
            uint32_t shndx;
        } elf_sec;
    };
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint32_t framebuffer_bpp;
    uint32_t framebuffer_type;
} multiboot_info_t;

// Multiboot memory map structure for 32-bit mode
typedef struct {
    uint32_t size;
    uint32_t base_addr; // 32-bit base address
    uint32_t length;    // 32-bit length
    uint32_t type;
} multiboot_memory_map_t;

// Multiboot header
typedef struct {
    uint32_t magic;
    uint32_t flags;
    uint32_t checksum;
} multiboot_header_t;

void print_multiboot_info(const multiboot_info_t *mb_info);

#endif // MULTIBOOT_H
