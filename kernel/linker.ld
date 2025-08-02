OUTPUT_FORMAT(elf32-i386)
ENTRY(start)

SECTIONS
{
    . = 0x00100000;

    _kernel_start = .;

    .multiboot ALIGN(4) : {
        *(.multiboot)
    }

    .text ALIGN(0x1000) : {
        *(.text)
    }

    .rodata ALIGN(0x1000) : {
        *(.rodata)
    }

    .data ALIGN(0x1000) : {
        *(.data)
    }

    .bss ALIGN(0x1000) : {
        *(.bss)
        *(COMMON)
    }

    _kernel_end = .;
}
