#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>

// Multiboot header magic number
#define MULTIBOOT_MAGIC 0x1BADB002

// Multiboot flags
#define MULTIBOOT_INFO_MEMORY         0x00000001
#define MULTIBOOT_INFO_BOOTDEV        0x00000002
#define MULTIBOOT_INFO_CMDLINE        0x00000004
#define MULTIBOOT_INFO_MODS           0x00000008
#define MULTIBOOT_INFO_AOUT_SYMS      0x00000010
#define MULTIBOOT_INFO_ELF_SHDR       0x00000020
#define MULTIBOOT_INFO_MEM_MAP        0x00000040
#define MULTIBOOT_INFO_DRIVE_INFO     0x00000080
#define MULTIBOOT_INFO_CONFIG_TABLE   0x00000100
#define MULTIBOOT_INFO_BOOT_LOADER    0x00000200
#define MULTIBOOT_INFO_APM_TABLE      0x00000400
#define MULTIBOOT_INFO_FRAMEBUFFER    0x00000800

// Multiboot information structure
typedef struct {
    uint32_t flags;                // Flags indicating available fields
    uint32_t mem_lower;            // Lower memory size (in KB)
    uint32_t mem_upper;            // Upper memory size (in KB)
    uint32_t boot_device;          // Boot device ID
    uint32_t cmdline;              // Address of the kernel command line
    uint32_t mods_count;           // Number of modules loaded
    uint32_t mods_addr;            // Address of the first module
    union {
        struct {                   // a.out symbol table (if applicable)
            uint32_t tabsize;      // Table size
            uint32_t strsize;      // String size
            uint32_t addr;         // Table address
        } aout_sym;
        struct {                   // ELF section header table (if applicable)
            uint32_t num;          // Number of entries
            uint32_t size;         // Size of each entry
            uint32_t addr;         // Address of the table
            uint32_t shndx;        // Index of the string table
        } elf_sec;
    };
    uint32_t mmap_length;          // Length of the memory map
    uint32_t mmap_addr;            // Address of the memory map
    uint32_t drives_length;        // Length of the drive information
    uint32_t drives_addr;          // Address of the drive information
    uint32_t config_table;         // Address of the ROM configuration table
    uint32_t boot_loader_name;     // Address of the bootloader name
    uint32_t apm_table;            // Address of the APM table
    uint32_t framebuffer_addr;     // Framebuffer address
    uint32_t framebuffer_pitch;    // Framebuffer pitch
    uint32_t framebuffer_width;    // Framebuffer width
    uint32_t framebuffer_height;   // Framebuffer height
    uint32_t framebuffer_bpp;      // Framebuffer bits per pixel
    uint32_t framebuffer_type;     // Framebuffer type
} multiboot_info_t;

// Multiboot memory map structure
typedef struct {
    uint32_t size;                 // Size of this structure (excluding this field)
    uint64_t base_addr;            // Base address of the memory region
    uint64_t length;               // Length of the memory region
    uint32_t type;                 // Type of the memory region (1 = usable)
} __attribute__((packed)) multiboot_memory_map_t;

// Multiboot header structure
typedef struct {
    uint32_t magic;                // Magic number (must be MULTIBOOT_MAGIC)
    uint32_t flags;                // Feature flags
    uint32_t checksum;             // Checksum (magic + flags + checksum = 0)
} multiboot_header_t;

// Print Multiboot information (for debugging purposes)
void print_multiboot_info(const multiboot_info_t *mb_info);
void test_multiboot_framebuffer(const multiboot_info_t *mb_info) ;
#endif // MULTIBOOT_H
