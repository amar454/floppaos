# 
# Copyright 2024-2025 Amar Djulovic <aaamargml@gmail.com>
#
# This file is part of FloppaOS.
#
# FloppaOS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
#
# FloppaOS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with FloppaOS. If not, see <https://www.gnu.org/licenses/>.
#
# ------------------------------------------------------------------------------
#
# Makefile
#
#   This is the Makefile for FloppaOS.
#   It defines the build process for the operating system, including compiling source files, linking object files, and creating an ISO image.
#   It also includes rules for cleaning up build artifacts and creating a bootable USB drive.
#   The Makefile uses various tools such as gcc, nasm, ld, and grub-mkrescue to build the operating system.
#   It also includes comments and instructions for using the Makefile.
#   The Makefile is designed to be used with the GNU Make tool.
#   The Makefile is intended to be used on a Unix-like system. 
#   Windows is not supported by default and you must use a program like the Windows Subsystem for Linux (WSL) to use this Makefile.
#
# ------------------------------------------------------------------------------

# Define commands
CP := cp
RM := rm -rf
MKDIR := mkdir -pv
FIND := find

# Define output and paths
BIN = kernel
CFG = grub.cfg
ISO_PATH := iso
BOOT_PATH := $(ISO_PATH)/boot
GRUB_PATH := $(BOOT_PATH)/grub

# Compiler flags and linker settings
CFLAGS = -m32 -ffreestanding -fno-stack-protector -std=c11
ISR_CFLAGS = $(CFLAGS) -mgeneral-regs-only
LD_FLAGS = -m elf_i386 -T linker.ld

# Source files and directories
ASM_FILES = \
    boot.asm \

C_FILES = \
    kernel.c \
    apps/echo.c \
    fshell/fshell.c \
    fs/flopfs/flopfs.c \
    fs/tmpflopfs/tmpflopfs.c \
    lib/str.c \
    drivers/vga/vgahandler.c \
    drivers/time/floptime.c \
    fs/tmpflopfs/fileutils.c \
    mem/vmm.c \
    mem/pmm.c \
    mem/paging.c \
    mem/utils.c \
    drivers/io/io.c \
    task/task_handler.c \
    lib/flopmath.c \
    apps/dsp/dsp.c \
    drivers/vga/framebuffer.c \
    multiboot/multiboot.c \
    apps/floptxt/floptxt.c \
    drivers/acpi/acpi.c \
    drivers/mouse/ps2ms.c \
    mem/gdt.c \
    lib/logging.c \
    mem/kalloc.c

# Interrupt Service Routines, compiled with the gcc -mgeneral-regs-only flag to avoid using xmm registers
ISR_FILES = \
    interrupts/interrupts.c \
    drivers/keyboard/keyboard.c
# Dependencies
DEPENDENCIES = gcc nasm ld grub-mkrescue

# Object files
C_OBJ_FILES = $(C_FILES:.c=.o)
C_ISR_OBJ_FILES = $(ISR_FILES:.c=.o)
ASM_OBJ_FILES = $(ASM_FILES:.asm=.o)
OBJ_FILES = boot.o $(C_OBJ_FILES) $(C_ISR_OBJ_FILES)


# Main build target
.PHONY: all
all: bootloader kernel isr linker iso
	@echo "Build completed successfully."
# Check for dependencies
.PHONY: check_dependencies
check_dependencies:
	@for dep in $(DEPENDENCIES); do \
		if ! command -v $$dep &> /dev/null; then \
			echo "Error: $$dep is not installed."; \
			exit 1; \
		fi \
	done

# Bootloader compilation
bootloader: $(ASM_FILES) 
	nasm -f elf32 boot.asm -o boot.o 
	
# Kernel compilation
kernel: $(C_FILES)
	@for file in $(C_FILES); do \
		gcc $(CFLAGS) -c $$file -o $${file%.c}.o; \
	done

# ISR compilation
isr: $(ISR_FILES)
	@for file in $(ISR_FILES); do \
		gcc $(ISR_CFLAGS) -c $$file -o $${file%.c}.o; \
	done

# Linker step
linker: $(OBJ_FILES)
	ld $(LD_FLAGS) -o $(BIN) $(C_OBJ_FILES) $(C_ISR_OBJ_FILES) $(ASM_OBJ_FILES) 



# ISO creation
iso: $(BIN)
	$(MKDIR) $(GRUB_PATH)
	$(CP) $(BIN) $(BOOT_PATH)
	$(CP) $(CFG) $(GRUB_PATH)
	grub-file --is-x86-multiboot $(BOOT_PATH)/$(BIN)
	grub-mkrescue -o floppaOS-alpha.iso $(ISO_PATH)

# Clean build artifacts
.PHONY: clean cleanobj

# clean: Remove all build artifacts
clean:
	$(RM) $(BIN) *.iso $(ISO_PATH)
	$(FIND) . -name "*.o" -exec $(RM) {} \;
# cleanobj: Remove only object files
cleanobj:
	$(FIND) . -name "*.o" -exec $(RM) {} \;

