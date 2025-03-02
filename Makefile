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
CC = gcc
LD = ld
NASM = nasm
CFLAGS = -m32 -ffreestanding -fno-stack-protector -std=c11 -Wall -Wextra
INTERRUPT_FLAGS = -m32 -ffreestanding -fno-stack-protector -std=c11 -Wall -Wextra -mgeneral-regs-only
LD_FLAGS = -m elf_i386 -T linker.ld

# Source files and directories
ASM_FILES = boot.asm

C_FILES = \
    kernel.c \
    apps/echo.c \
    fshell/fshell.c \
    fs/flopfs/flopfs.c \
    fs/tmpflopfs/tmpflopfs.c \
    lib/str.c \
    drivers/vga/vgahandler.c \
    drivers/keyboard/keyboard.c \
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
    mem/alloc.c

OBJ_FILES = boot.o $(C_FILES:.c=.o) interrupts.o

# Clean build artifacts
.PHONY: clean cleanobj
clean:
	$(RM) $(BIN) *.iso $(ISO_PATH)
	$(FIND) . -name "*.o" -exec $(RM) {} \;

cleanobj:
	$(FIND) . -name "*.o" -exec $(RM) {} \;

# Main build target
.PHONY: all
all: bootloader kernel interrupts linker iso
	@echo "Build completed successfully."

# Bootloader compilation
bootloader: $(ASM_FILES)
	$(NASM) -f elf32 boot.asm -o boot.o 

# Kernel compilation (excluding interrupts.c)
kernel: $(C_FILES)
	$(CC) $(CFLAGS) -c $(C_FILES)

# Compile interrupts.c separately with special flags
interrupts: interrupts/interrupts.c
	$(CC) $(INTERRUPT_FLAGS) -c interrupts/interrupts.c -o interrupts.o

# Linker step
linker: $(OBJ_FILES)
	$(LD) $(LD_FLAGS) -o $(BIN) $(OBJ_FILES)

# ISO creation
iso: $(BIN)
	$(MKDIR) $(GRUB_PATH)
	$(CP) $(BIN) $(BOOT_PATH)
	$(CP) $(CFG) $(GRUB_PATH)
	grub-file --is-x86-multiboot $(BOOT_PATH)/$(BIN)
	grub-mkrescue -o floppaOS-alpha.iso $(ISO_PATH)

.PHONY: qemu

# Run the OS in QEMU
qemu: all
	qemu-system-i386 -cdrom floppaOS-alpha.iso
