#ifndef KERNEL_H
#define KERNEL_H

#define VGA_ADDRESS 0xB8000   /* Video memory begins here. */


// Declare these variables as extern
extern unsigned short *terminal_buffer; // External declaration
extern unsigned int vga_index; // External declaration

void clear_screen(void);
int main(void);

#endif // KERNEL_H
