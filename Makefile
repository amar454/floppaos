
CP := cp
RM := rm -rf
MKDIR := mkdir -pv
FIND := find

BIN = kernel
CFG = grub.cfg
ISO_PATH := iso
BOOT_PATH := $(ISO_PATH)/boot
GRUB_PATH := $(BOOT_PATH)/grub
BUILD_PATH := build

CC = gcc
LD = ld
NASM = nasm
CFLAGS = \
		-m32 \
		-ffreestanding \
		-fno-stack-protector \
		-std=gnu2x \
		-g
CSTOPONFIRSTERROR_FLAG = -Wfatal-errors
CWARNINGS = -Werror                                \
			-Wall                                  \
			-Wextra                                \
			-Wno-unused-parameter                  \
			-Wno-unused-variable                   \
			-Wno-unused-function                   \
			-Wno-unused-but-set-variable           \
			-Wno-missing-field-initializers        \
			-Wno-sign-compare                      \
			-Wno-format-truncation                 \
			-Wno-format-overflow                   \
			-Wno-format-extra-args                 \
			-Wno-format-zero-length                \
			-Wno-maybe-uninitialized               \
			-Wno-implicit-fallthrough				
INTERRUPT_FLAGS = $(CFLAGS) -mgeneral-regs-only
LD_FLAGS = -m elf_i386 -T linker.ld

SCHED_SRC = \
	task/sched.c \
	task/thread.c \
	task/pid.c \
	task/sync/mutex.c \
	task/sync/spinlock.c 

MEM_SRC = \
	mem/vmm.c \
	mem/pmm.c \
	mem/paging.c \
	mem/utils.c \
	mem/gdt.c \
	mem/alloc.c \
	mem/slab.c

DRIVER_SRC = \
	drivers/vga/vgahandler.c \
	drivers/keyboard/keyboard.c \
	drivers/time/floptime.c \
	drivers/io/io.c \
	drivers/vga/framebuffer.c \
	drivers/acpi/acpi.c \
	drivers/mouse/ps2ms.c

FS_SRC = \
	fs/flopfs/flopfs.c \
	fs/tmpflopfs/tmpflopfs.c \
	fs/tmpflopfs/fileutils.c

LIB_SRC = \
	lib/str.c \
	lib/flopmath.c \
	lib/logging.c

APP_SRC = \
	apps/echo.c \
	apps/dsp/dsp.c

OTHER_SRC = \
	kernel.c \
	multiboot/multiboot.c \
	fshell/fshell.c 

ASM_SRC = boot.asm task/context_switch.asm task/setup_stack.asm task/thread_entry.asm
FLANTERM_SRC = \
	flanterm/flanterm.c \
	flanterm/backends/fb.c 
C_SRC =  \
    $(SCHED_SRC)  \
	$(MEM_SRC)    \
	$(DRIVER_SRC) \
	$(FS_SRC)     \
	$(LIB_SRC)    \
	$(APP_SRC)    \
	$(OTHER_SRC)  \
	$(FLANTERM_SRC)

OBJ_SRC = $(addprefix $(BUILD_PATH)/, boot.o task/context_switch.o task/setup_stack.o task/thread_entry.o $(C_SRC:.c=.o) interrupts.o)

.PHONY: clean cleanobj all bootloader context_switch kernel  interrupts linker iso qemu qemu-monitor qemu-log

clean:
	$(RM) $(BIN) $(ISO_PATH) $(BUILD_PATH)

cleanobj:
	$(RM) $(BUILD_PATH) 

# Main build target
all: $(BUILD_PATH) bootloader kernel flanterm interrupts linker iso
	@echo "Build completed successfully."

# Create build directory
$(BUILD_PATH):
	$(MKDIR) $(BUILD_PATH)

# make boot entry assembly file
bootloader: $(ASM_SRC) | $(BUILD_PATH)
	$(NASM) -f elf32 boot.asm -o $(BUILD_PATH)/boot.o

# Kernel compilation routines
kernel: sched sched_asm mem drivers fs lib apps other | $(BUILD_PATH)

$(BUILD_PATH)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CWARNINGS) -c $< -o $@


sched: $(addprefix $(BUILD_PATH)/, $(SCHED_SRC:.c=.o))

mem: $(addprefix $(BUILD_PATH)/, $(MEM_SRC:.c=.o))

drivers: $(addprefix $(BUILD_PATH)/, $(DRIVER_SRC:.c=.o))

fs: $(addprefix $(BUILD_PATH)/, $(FS_SRC:.c=.o))

lib: $(addprefix $(BUILD_PATH)/, $(LIB_SRC:.c=.o))

apps: $(addprefix $(BUILD_PATH)/, $(APP_SRC:.c=.o))

other: $(addprefix $(BUILD_PATH)/, $(OTHER_SRC:.c=.o))

sched_asm: task/context_switch.asm task/setup_stack.asm | $(BUILD_PATH)
	@$(MKDIR) $(BUILD_PATH)/task
	$(NASM) -f elf32 task/context_switch.asm -o $(BUILD_PATH)/task/context_switch.o
	$(NASM) -f elf32 task/setup_stack.asm -o $(BUILD_PATH)/task/setup_stack.o
	$(NASM) -f elf32 task/thread_entry.asm -o $(BUILD_PATH)/task/thread_entry.o
interrupts: interrupts/interrupts.c | $(BUILD_PATH)

	@mkdir -p $(BUILD_PATH)/interrupts
	$(CC) $(INTERRUPT_FLAGS) $(CWARNINGS) -c interrupts/interrupts.c -o $(BUILD_PATH)/interrupts.o
flanterm: $(addprefix $(BUILD_PATH)/, $(FLANTERM_SRC:.c=.o))
	@mkdir -p $(BUILD_PATH)/flanterm
	$(CC) $(CFLAGS) $(CWARNINGS) -c flanterm/flanterm.c -o $(BUILD_PATH)/flanterm.o
	$(CC) $(CFLAGS) $(CWARNINGS) -c flanterm/backends/fb.c -o $(BUILD_PATH)/fb.o
linker: $(OBJ_SRC)
	$(LD) $(LD_FLAGS) -o $(BIN) $(OBJ_SRC)

iso: $(BIN)
	$(MKDIR) $(GRUB_PATH)
	$(CP) $(BIN) $(BOOT_PATH)
	$(CP) $(CFG) $(GRUB_PATH)
	grub-file --is-x86-multiboot $(BOOT_PATH)/$(BIN)
	grub-mkrescue -o $(ISO_PATH)/floppaOS-alpha.iso $(ISO_PATH)

fatal: $(C_SRC) interrupts/interrupts.c | $(BUILD_PATH)
	@for file in $(C_SRC); do \
		mkdir -p $(BUILD_PATH)/$(dirname $file); \
		$(CC) $(CFLAGS) $(CSTOPONFIRSTERROR_FLAG) -c $file -o $(BUILD_PATH)/${file%.c}.o; \
	done
	@mkdir -p $(BUILD_PATH)/interrupts
	$(CC) $(INTERRUPT_FLAGS) $(CSTOPONFIRSTERROR_FLAG) -c interrupts/interrupts.c -o $(BUILD_PATH)/interrupts.o

# QEMU targets
qemu: clean all
	qemu-system-i386 -cdrom $(ISO_PATH)/floppaOS-alpha.iso --no-shutdown

qemu-monitor: clean all
	qemu-system-i386 -cdrom $(ISO_PATH)/floppaOS-alpha.iso --no-shutdown -monitor stdio

qemu-log-all: clean all
	qemu-system-i386 -d int,cpu_reset,exec -cdrom $(ISO_PATH)/floppaOS-alpha.iso

qemu-log-int: clean all
	qemu-system-i386 -d int -cdrom $(ISO_PATH)/floppaOS-alpha.iso -monitor stdio

qemu-log-int-serial: clean all
	qemu-system-i386 -d int,cpu_reset,exec -cdrom $(ISO_PATH)/floppaOS-alpha.iso -monitor stdio -serial stdio

qemu-log-exec: clean all
	qemu-system-i386 -d int,cpu_reset,exec -cdrom $(ISO_PATH)/floppaOS-alpha.iso -monitor stdio -serial stdio -s -S