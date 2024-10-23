ENTRY(start)
SECTIONS
{
    /* Set the address of the bootloader to 0x7C00 */
    . = 0x7C00;

    .text :
    {
        *(.text)
    }

    /* Align the binary to 512 bytes (1 sector) */
    /DISCARD/ : { *(.eh_frame) }
    . = 0x7DFE;
    SHORT(0xAA55);  /* Bootloader signature at the end of the first sector */
}
