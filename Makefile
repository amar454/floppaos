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
CFLAGS = -m32 -ffreestanding -fno-stack-protector
LD_FLAGS = -m elf_i386 -T linker.ld

# Object files
OBJ_FILES = \
    boot.o \
    kernel.o \
    apps/echo.o \
    fshell/fshell.o \
    fs/flopfs/flopfs.o \
    fs/tmpflopfs/tmpflopfs.o \
    lib/str.o \
    drivers/vga/vgahandler.o \
    drivers/keyboard/keyboard.o \
    drivers/time/floptime.o \
    fs/tmpflopfs/fileutils.o \
    mem/memutils.o \
    drivers/io/io.o \
    task/task_handler.o \
    lib/flopmath.o \
    apps/dsp/dsp.o \
    drivers/vga/framebuffer.o \
    multiboot/multiboot.o \
    apps/floptxt/floptxt.o \
    drivers/acpi/acpi.o \
    interrupts/interrupts.o \
    drivers/mouse/ps2ms.o

    

# Dependency check
CHECK_DEPENDENCIES = \
    nasm gcc ld grub-mkrescue grub-file

# Check dependencies
.PHONY: check-dependencies
check-dependencies:
	@echo "Checking dependencies..."
	@for dep in $(CHECK_DEPENDENCIES); do \
	    if ! command -v $$dep &>/dev/null; then \
	        echo "Error: $$dep is not installed."; \
	        echo "Please install it using your package manager."; \
		echo "Try 'make install_guide' for information for your system."; \
	        exit 1; \
	    fi; \
	done
	@echo "All dependencies are installed."

# Main build target
.PHONY: all
all: check-dependencies bootloader kernel linker iso
	@echo "Make has completed."

# Bootloader compilation
bootloader: boot.asm
	nasm -f elf32 boot.asm -o boot.o

# Kernel compilation
kernel: \
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
    mem/memutils.c \
    drivers/io/io.c \
    task/task_handler.c \
    lib/flopmath.c \
    apps/dsp/dsp.c \
    drivers/vga/framebuffer.c \
    multiboot/multiboot.c \
    apps/floptxt/floptxt.c \
    drivers/acpi/acpi.c \
    interrupts/interrupts.c \
    drivers/mouse/ps2ms.c
	gcc $(CFLAGS) -c \
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
        mem/memutils.c \
        drivers/io/io.c \
        task/task_handler.c \
        lib/flopmath.c \
        apps/dsp/dsp.c \
        drivers/vga/framebuffer.c \
        multiboot/multiboot.c \
        apps/floptxt/floptxt.c \
        drivers/acpi/acpi.c \
        interrupts/interrupts.c \
        drivers/mouse/ps2ms.c
# Linker step
linker: linker.ld $(OBJ_FILES)
	ld $(LD_FLAGS) -o $(BIN) $(OBJ_FILES)

# ISO creation
iso: $(BIN)
	$(MKDIR) $(GRUB_PATH)
	$(CP) $(BIN) $(BOOT_PATH)
	$(CP) $(CFG) $(GRUB_PATH)
	grub-file --is-x86-multiboot $(BOOT_PATH)/$(BIN)
	grub-mkrescue -o floppaOS-alpha.iso $(ISO_PATH)

# Clean build artifacts
.PHONY: clean cleanobj
clean:
	$(RM) $(BIN) *.iso $(ISO_PATH)
	$(FIND) . -name "*.o" -exec $(RM) {} \;

cleanobj:
	$(FIND) . -name "*.o" -exec $(RM) {} \;

# Installation guide
.PHONY: install-guide
install-guide:
	@echo "----------------------------------------------------"
	@echo "Installation Guide for Dependencies:"
	@echo "----------------------------------------------------"
	@echo "1. On Ubuntu/Debian-based systems:"
	@echo "   sudo apt update"
	@echo "   sudo apt install nasm gcc grub-pc-bin grub2-common"
	@echo "   sudo apt install make xorriso"
	@echo ""
	@echo "2. On Fedora/RHEL/CentOS-based systems:"
	@echo "   sudo dnf install nasm gcc grub2-tools"
	@echo "   sudo dnf install make xorriso"
	@echo ""
	@echo "3. On Arch Linux-based systems:"
	@echo "   sudo pacman -S nasm gcc grub dosfstools"
	@echo "   sudo pacman -S make xorriso"
	@echo ""
	@echo "After installing the dependencies, run 'make' to build the project."
	@echo "----------------------------------------------------"