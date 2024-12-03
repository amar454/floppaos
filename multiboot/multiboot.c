#include "multiboot.h"
#include "../apps/echo.h" 
#include "../drivers/vga/vgahandler.h"
/*void print_multiboot_info(const multiboot_info_t *mb_info) {
    if (mb_info->flags & MULTIBOOT_INFO_MEMORY) {
        echo("Memory Info:\n", WHITE);
        echo_f("  Lower Memory: %u KB\n", WHITE, mb_info->mem_lower);
        echo_f("  Upper Memory: %u KB\n", WHITE, mb_info->mem_upper);
    }

    if (mb_info->flags & MULTIBOOT_INFO_BOOTDEV) {
        echo("Boot Device Info:\n", WHITE);
        echo_f("  Boot Device: 0x%x\n", WHITE, mb_info->boot_device);
    }

    if (mb_info->flags & MULTIBOOT_INFO_CMDLINE) {
        echo("Command Line:\n", WHITE);
        echo_f("  %s\n", WHITE, (const char *)mb_info->cmdline);
    }

    if (mb_info->flags & MULTIBOOT_INFO_MODS) {
        echo("Modules Info:\n", WHITE);
        echo_f("  Modules Count: %u\n", WHITE, mb_info->mods_count);
        echo_f("  Modules Address: 0x%x\n", WHITE, mb_info->mods_addr);
    }

    if (mb_info->flags & MULTIBOOT_INFO_AOUT_SYMS) {
        echo("AOUT Symbols Info:\n", WHITE);
        echo_f("  Tab Size: %u\n", WHITE, mb_info->aout_sym.tabsize);
        echo_f("  String Size: %u\n", WHITE, mb_info->aout_sym.strsize);
        echo_f("  Address: 0x%x\n", WHITE, mb_info->aout_sym.addr);
    }

    if (mb_info->flags & MULTIBOOT_INFO_ELF_SHDR) {
        echo("ELF Section Info:\n", WHITE);
        echo_f("  Number: %u\n", WHITE, mb_info->elf_sec.num);
        echo_f("  Size: %u\n", WHITE, mb_info->elf_sec.size);
        echo_f("  Address: 0x%x\n", WHITE, mb_info->elf_sec.addr);
        echo_f("  Shndx: %u\n", WHITE, mb_info->elf_sec.shndx);
    }

    if (mb_info->flags & MULTIBOOT_INFO_MEM_MAP) {
        echo("Memory Map:\n", WHITE);
        multiboot_memory_map_t *mmap = (multiboot_memory_map_t *)mb_info->mmap_addr;
        while ((uint32_t)mmap < mb_info->mmap_addr + mb_info->mmap_length) {
            echo_f("  Base: 0x%llx, Length: 0x%llx, Type: %u\n", WHITE,
                   mmap->base_addr, mmap->length, mmap->type);
            mmap = (multiboot_memory_map_t *)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
        }
    }

    if (mb_info->flags & MULTIBOOT_INFO_DRIVE_INFO) {
        echo("Drive Info:\n", WHITE);
        echo_f("  Drives Length: %u\n", WHITE, mb_info->drives_length);
        echo_f("  Drives Address: 0x%x\n", WHITE, mb_info->drives_addr);
    }

    if (mb_info->flags & MULTIBOOT_INFO_CONFIG_TABLE) {
        echo("Config Table:\n", WHITE);
        echo_f("  Address: 0x%x\n", WHITE, mb_info->config_table);
    }

    if (mb_info->flags & MULTIBOOT_INFO_BOOT_LOADER) {
        echo("Boot Loader Name:\n", WHITE);
        echo_f("  %s\n", WHITE, (const char *)mb_info->boot_loader_name);
    }

    if (mb_info->flags & MULTIBOOT_INFO_APM_TABLE) {
        echo("APM Table Info:\n", WHITE);
        echo_f("  Address: 0x%x\n", WHITE, mb_info->apm_table);
    }

    if (mb_info->flags & MULTIBOOT_INFO_FRAMEBUFFER) {
        echo("Framebuffer Info:\n", WHITE);
        echo_f("  Address: 0x%x\n", WHITE, mb_info->framebuffer_addr);
        echo_f("  Pitch: %u\n", WHITE, mb_info->framebuffer_pitch);
        echo_f("  Width: %u\n", WHITE, mb_info->framebuffer_width);
        echo_f("  Height: %u\n", WHITE, mb_info->framebuffer_height);
        echo_f("  BPP: %u\n", WHITE, mb_info->framebuffer_bpp);
        echo_f("  Type: %u\n", WHITE, mb_info->framebuffer_type);
    }
}*/