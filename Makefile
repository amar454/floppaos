CP := cp
RM := rm -rf
MKDIR := mkdir -pv

BIN = kernel
CFG = grub.cfg
ISO_PATH := iso
BOOT_PATH := $(ISO_PATH)/boot
GRUB_PATH := $(BOOT_PATH)/grub

CFLAGS = -m32 -ffreestanding -fno-stack-protector
LD_FLAGS = -m elf_i386 -T linker.ld

# Object files
OBJ_FILES = boot.o kernel.o echo.o fshell.o filesystem.o strutils.o vgacolors.o keyboard.o

.PHONY: all
all: bootloader kernel linker iso
	@echo Make has completed.

bootloader: boot.asm
	nasm -f elf32 boot.asm -o boot.o

kernel: kernel.c echo.c fshell.c filesystem.c strutils.c vgacolors.c keyboard.c
	gcc $(CFLAGS) -c kernel.c echo.c fshell.c filesystem.c strutils.c vgacolors.c keyboard.c

linker: linker.ld $(OBJ_FILES)
	ld $(LD_FLAGS) -o $(BIN) $(OBJ_FILES)

iso: $(BIN)
	$(MKDIR) $(GRUB_PATH)
	$(CP) $(BIN) $(BOOT_PATH)
	$(CP) $(CFG) $(GRUB_PATH)
	grub-file --is-x86-multiboot $(BOOT_PATH)/$(BIN)
	grub-mkrescue -o my-kernel.iso $(ISO_PATH)

.PHONY: clean
clean:
	$(RM) *.o $(BIN) *.iso $(ISO_PATH)
