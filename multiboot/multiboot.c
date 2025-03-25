#include "multiboot.h"
#include "../apps/echo.h" 
#include "../drivers/vga/vgahandler.h"
#include "../drivers/time/floptime.h"
#include "../lib/logging.h"


void print_multiboot_info(multiboot_info_t* mb_info) {
    log_step("Multiboot Information:\n", YELLOW);
    log_uint("Flags: ", mb_info->flags);

    if (mb_info->flags & MULTIBOOT_INFO_MEMORY) {

        log_uint("Memory Lower (KB): ", mb_info->mem_lower);
        log_uint("Memory Upper (KB): ", mb_info->mem_upper);
    }

    if (mb_info->flags & MULTIBOOT_INFO_BOOTDEV) {
        log_uint("Boot Device: ", mb_info->boot_device);
    }

    if (mb_info->flags & MULTIBOOT_INFO_CMDLINE) {
        log_address("Command Line Address: ", mb_info->cmdline);
    }

    if (mb_info->flags & MULTIBOOT_INFO_MODS) {
        log_uint("Modules Count:\n ", mb_info->mods_count);
        log_address("Modules Address: ", mb_info->mods_addr);
    }

    if (mb_info->flags & MULTIBOOT_INFO_AOUT_SYMS) {
        log_step("AOUT Symbol Table:\n", LIGHT_GRAY);
        log_uint("Tab Size: ", mb_info->u.aout_sym.tabsize);
        log_uint("Str Size: ", mb_info->u.aout_sym.strsize);
        log_address("Address: ", mb_info->u.aout_sym.addr);
    }

    if (mb_info->flags & MULTIBOOT_INFO_ELF_SHDR) {
        log_step("ELF Section Header Table\n", LIGHT_GRAY);
        log_uint("Number of Entries: ", mb_info->u.elf_sec.num);
        log_uint("Size of Entry: ", mb_info->u.elf_sec.size);
        log_address("Address: ", mb_info->u.elf_sec.addr);
        log_uint("Index of Section Names: ", mb_info->u.elf_sec.shndx);
    }

    if (mb_info->flags & MULTIBOOT_INFO_MEM_MAP) {
        log_step("Memory Map:\n", LIGHT_GRAY);
        log_uint("Memory Map Length: ", mb_info->mmap_length);
        log_address("Memory Map Address: ", mb_info->mmap_addr);
    }

    if (mb_info->flags & MULTIBOOT_INFO_DRIVE_INFO) {
        log_uint("Drives Length: ", mb_info->drives_length);
        log_address("Drives Address: ", mb_info->drives_addr);
    }

    if (mb_info->flags & MULTIBOOT_INFO_CONFIG_TABLE) {
        log_address("Config Table Address: ", mb_info->config_table);
    }

    if (mb_info->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME) {
        log_address("Boot Loader Name Address: ", mb_info->boot_loader_name);
    }

    if (mb_info->flags & MULTIBOOT_INFO_APM_TABLE) {
        log_address("APM Table Address: ", mb_info->apm_table);
    }

    if (mb_info->flags & MULTIBOOT_INFO_VBE_INFO) {
        log_address("VBE Control Info: ", mb_info->vbe_control_info);
        log_address("VBE Mode Info: ", mb_info->vbe_mode_info);
        log_uint("VBE Mode: ", mb_info->vbe_mode);
        log_uint("VBE Interface Segment: ", mb_info->vbe_interface_seg);
        log_uint("VBE Interface Offset: ", mb_info->vbe_interface_off);
        log_uint("VBE Interface Length: ", mb_info->vbe_interface_len);
    }

    if (mb_info->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO) {
        log_step("Framebuffer Info:\n", LIGHT_GRAY);
        log_address("Framebuffer Address: ", (uint32_t)mb_info->framebuffer_addr);
        log_uint("Framebuffer Pitch: ", mb_info->framebuffer_pitch);
        log_uint("Framebuffer Width: ", mb_info->framebuffer_width);
        log_uint("Framebuffer Height: ", mb_info->framebuffer_height);
        log_uint("Framebuffer Bits Per Pixel: ", mb_info->framebuffer_bpp);
        log_uint("Framebuffer Type: ", mb_info->framebuffer_type);
    }

    log_step("Done printing multiboot info.\n", GREEN);
}