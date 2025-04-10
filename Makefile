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
BUILD_PATH := build

# Compiler flags and linker settings 

CC = gcc
LD = ld
NASM = nasm
CFLAGS = -m32 -ffreestanding -fno-stack-protector -std=c11 
CWARNINGS = -Wno-unused-variable -Wno-unused-function -Wno-unused-label -Wno-unused-parameter -Wno-unused-value -Wno-format
INTERRUPT_FLAGS = -m32 -ffreestanding -fno-stack-protector -std=c2x -Wall -Wextra -mgeneral-regs-only 
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
    mem/alloc.c \
    mem/slab.c 

OBJ_FILES = $(addprefix $(BUILD_PATH)/, boot.o $(C_FILES:.c=.o) interrupts.o)

# Clean build artifacts
.PHONY: clean cleanobj
clean:
	$(RM) $(BIN) $(ISO_PATH) $(BUILD_PATH)

cleanobj:
	$(RM) $(BUILD_PATH)

# Main build target
.PHONY: all bootloader kernel interrupts linker iso

all: $(BUILD_PATH) bootloader kernel interrupts linker iso
	@echo "Build completed successfully."


all-no-warn : $(BUILD_PATH) bootloader kernel-no-warn interrupts linker iso
	@echo "Build completed successfully."


$(BUILD_PATH):
	$(MKDIR) $(BUILD_PATH)

# Bootloader compilation
bootloader: $(ASM_FILES) | $(BUILD_PATH)
	$(NASM) -f elf32 boot.asm -o $(BUILD_PATH)/boot.o 

# Kernel compilation (excluding interrupts.c) 
kernel: $(C_FILES) | $(BUILD_PATH)
	@for file in $(C_FILES); do \
		mkdir -p $(BUILD_PATH)/$$(dirname $$file); \
		$(CC) $(CFLAGS)$(CWARNINGS) -c $$file -o $(BUILD_PATH)/$${file%.c}.o; \
	done
# Kernel warning free compilation (excluding interrupts.c) 
kernel-no-warn: $(C_FILES) | $(BUILD_PATH)
	@for file in $(C_FILES); do \
		mkdir -p $(BUILD_PATH)/$$(dirname $$file); \
		$(CC) $(CFLAGS)  -c $$file -o $(BUILD_PATH)/$${file%.c}.o; \
	done


# Compile interrupts.c separately with mgeneral-regs-only flag
interrupts: interrupts/interrupts.c | $(BUILD_PATH)
	@mkdir -p $(BUILD_PATH)/interrupts
	$(CC) $(INTERRUPT_FLAGS) -c interrupts/interrupts.c -o $(BUILD_PATH)/interrupts.o

# Linker step
linker: $(OBJ_FILES)
	$(LD) $(LD_FLAGS) -o $(BIN) $(OBJ_FILES)

# ISO creation
iso: $(BIN)
	$(MKDIR) $(GRUB_PATH)
	$(CP) $(BIN) $(BOOT_PATH)
	$(CP) $(CFG) $(GRUB_PATH)
	grub-file --is-x86-multiboot $(BOOT_PATH)/$(BIN)
	grub-mkrescue -o $(ISO_PATH)/floppaOS-alpha.iso $(ISO_PATH)

.PHONY: qemu, qemu-debug, qemu-log

# clean, compile, and run the OS in QEMU
qemu: clean all
	qemu-system-i386 -cdrom $(ISO_PATH)/floppaOS-alpha.iso --no-shutdown

# Run the OS in QEMU with logging
qemu-log: clean all
	qemu-system-i386 -d int,cpu_reset,exec -cdrom $(ISO_PATH)/floppaOS-alpha.iso