OUTPUT_FORMAT(elf32-i386)
ENTRY(start)
SECTIONS
{
    . = 0x00100000; /* Load address (1MB) */
    .multiboot ALIGN(4) : { *(.multiboot) }
    .text ALIGN(4) : { *(.text) }
    .rodata ALIGN(4) : { *(.rodata) }
    .data ALIGN(4) : { *(.data) }
    .bss ALIGN(4) : { *(.bss) }
}
