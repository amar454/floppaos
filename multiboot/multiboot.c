#include "multiboot.h"
#include "../apps/echo.h" 
#include "../drivers/vga/vgahandler.h"
#include "../drivers/time/floptime.h"
// Print the full multiboot information using echo_f
void print_multiboot_info(const multiboot_info_t *mb_info) {
    // Memory Info
    sleep_seconds(1);
    if (mb_info->flags & MULTIBOOT_INFO_MEMORY) {
        echo("Memory Info:\n", WHITE);
        echo_f("  Lower Memory: %u KB\n", WHITE, mb_info->mem_lower);
        echo_f("  Upper Memory: %u KB\n", WHITE, mb_info->mem_upper);
    }
    
    // Boot Device Info
    if (mb_info->flags & MULTIBOOT_INFO_BOOTDEV) {
        echo("Boot Device Info:\n", WHITE);
        echo_f("  Boot Device: 0x%x\n", WHITE, mb_info->boot_device);
    }
    
    // Command Line
    if (mb_info->flags & MULTIBOOT_INFO_CMDLINE) {
        echo("Command Line:\n", WHITE);
        // Cast cmdline address to const char *
        echo_f("  %s\n", WHITE, (const char *)(uintptr_t)mb_info->cmdline);
    }
    sleep_seconds(1);
    // Modules Info
    if (mb_info->flags & MULTIBOOT_INFO_MODS) {
        echo("Modules Info:\n", WHITE);
        echo_f("  Modules Count: %u\n", WHITE, mb_info->mods_count);
        echo_f("  Modules Address: 0x%x\n", WHITE, mb_info->mods_addr);
    }
    
    // AOUT Symbols Info
    if (mb_info->flags & MULTIBOOT_INFO_AOUT_SYMS) {
        echo("AOUT Symbols Info:\n", WHITE);
        echo_f("  Tab Size: %u\n", WHITE, mb_info->aout_sym.tabsize);
        echo_f("  String Size: %u\n", WHITE, mb_info->aout_sym.strsize);
        echo_f("  Address: 0x%x\n", WHITE, mb_info->aout_sym.addr);
    }
    sleep_seconds(1);
    // ELF Section Info
    if (mb_info->flags & MULTIBOOT_INFO_ELF_SHDR) {
        echo("ELF Section Info:\n", WHITE);
        echo_f("  Number: %u\n", WHITE, mb_info->elf_sec.num);
        echo_f("  Size: %u\n", WHITE, mb_info->elf_sec.size);
        echo_f("  Address: 0x%x\n", WHITE, mb_info->elf_sec.addr);
        echo_f("  Shndx: %u\n", WHITE, mb_info->elf_sec.shndx);
    }
    
    // Memory Map Info
    if (mb_info->flags & MULTIBOOT_INFO_MEM_MAP) {
        echo("Memory Map:\n", WHITE);
        multiboot_memory_map_t *mmap = (multiboot_memory_map_t *)(uintptr_t)mb_info->mmap_addr;
        while ((uintptr_t)mmap < (uintptr_t)(mb_info->mmap_addr + mb_info->mmap_length)) {
            echo_f("  Base: 0x%x, Length: 0x%x, Type: %u\n", WHITE,
                   mmap->base_addr, mmap->length, mmap->type);
            mmap = (multiboot_memory_map_t *)((uintptr_t)mmap + mmap->size + sizeof(mmap->size));
        }
    }
    sleep_seconds(1);
    // Drive Info
    if (mb_info->flags & MULTIBOOT_INFO_DRIVE_INFO) {
        echo("Drive Info:\n", WHITE);
        echo_f("  Drives Length: %u\n", WHITE, mb_info->drives_length);
        echo_f("  Drives Address: 0x%x\n", WHITE, mb_info->drives_addr);
    }
    
    // Config Table Info
    if (mb_info->flags & MULTIBOOT_INFO_CONFIG_TABLE) {
        echo("Config Table:\n", WHITE);
        echo_f("  Address: 0x%x\n", WHITE, mb_info->config_table);
    }
    
    // Boot Loader Name
    if (mb_info->flags & MULTIBOOT_INFO_BOOT_LOADER) {
        echo("Boot Loader Name:\n", WHITE);
        echo_f("  %s\n", WHITE, (const char *)(uintptr_t)mb_info->boot_loader_name);
    }
    
    // APM Table Info
    if (mb_info->flags & MULTIBOOT_INFO_APM_TABLE) {
        echo("APM Table Info:\n", WHITE);
        echo_f("  Address: 0x%x\n", WHITE, mb_info->apm_table);
    }
    sleep_seconds(1);
    // Framebuffer Info
    if (mb_info->flags & MULTIBOOT_INFO_FRAMEBUFFER) {
        echo("Framebuffer Info:\n", WHITE);
        echo_f("  Address: 0x%x\n", WHITE, mb_info->framebuffer_addr);
        echo_f("  Pitch: %u\n", WHITE, mb_info->framebuffer_pitch);
        echo_f("  Width: %u\n", WHITE, mb_info->framebuffer_width);
        echo_f("  Height: %u\n", WHITE, mb_info->framebuffer_height);
        echo_f("  BPP: %u\n", WHITE, mb_info->framebuffer_bpp);
        echo_f("  Type: %u\n", WHITE, mb_info->framebuffer_type);
    }
}
