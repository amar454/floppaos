OUTPUT_FORMAT(elf32-i386)
ENTRY(start)

SECTIONS
{
    . = 1M;                      /* Load kernel at 1MB */
    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.multiboot)            /* Multiboot header */
        *(.text)                 /* Kernel code */
    }
    .data : { *(.data) }         /* Kernel data */
    .bss  : { *(.bss) }          /* Kernel BSS */
}
