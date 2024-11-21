# Makefile

CP := cp
RM := rm -rf
MKDIR := mkdir -pv
FIND := find

BIN = kernel
CFG = grub.cfg
ISO_PATH := iso
BOOT_PATH := $(ISO_PATH)/boot
GRUB_PATH := $(BOOT_PATH)/grub

CFLAGS = -m32 -ffreestanding -fno-stack-protector
LD_FLAGS = -m elf_i386 -T linker.ld

# Object files
OBJ_FILES = boot.o kernel.o apps/echo.o fshell/fshell.o fs/flopfs/flopfs.o fs/tmpflopfs/tmpflopfs.o lib/str.o drivers/vga/vgacolors.o drivers/keyboard/keyboard.o drivers/time/floptime.o fs/tmpflopfs/fileutils.o mem/memutils.o drivers/io/io.o task/task_handler.o

.PHONY: all
all: bootloader kernel linker iso
	@echo Make has completed.

bootloader: boot.asm
	nasm -f elf32 boot.asm -o boot.o

kernel: kernel.c apps/echo.c fshell/fshell.c fs/flopfs/flopfs.c fs/tmpflopfs/tmpflopfs.c lib/str.c drivers/vga/vgacolors.c drivers/keyboard/keyboard.c drivers/time/floptime.c fs/tmpflopfs/fileutils.c mem/memutils.c drivers/io/io.c task/task_handler.c
	gcc $(CFLAGS) -c kernel.c apps/echo.c fshell/fshell.c fs/flopfs/flopfs.c fs/tmpflopfs/tmpflopfs.c lib/str.c drivers/vga/vgacolors.c drivers/keyboard/keyboard.c drivers/time/floptime.c fs/tmpflopfs/fileutils.c mem/memutils.c drivers/io/io.c task/task_handler.c

linker: linker.ld $(OBJ_FILES)
	ld $(LD_FLAGS) -o $(BIN) $(OBJ_FILES)

iso: $(BIN)
	$(MKDIR) $(GRUB_PATH)
	$(CP) $(BIN) $(BOOT_PATH)
	$(CP) $(CFG) $(GRUB_PATH)
	grub-file --is-x86-multiboot $(BOOT_PATH)/$(BIN)
	grub-mkrescue -o floppaOS-alpha.iso $(ISO_PATH)

.PHONY: clean cleanobj
clean:
	$(RM) $(BIN) *.iso $(ISO_PATH)
	$(FIND) . -name "*.o" -exec $(RM) {} \;

cleanobj:
	$(FIND) . -name "*.o" -exec $(RM) {} \;
