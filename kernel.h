#ifndef KERNEL_H
#define KERNEL_H

#include "multiboot/multiboot.h"
#define VGA_ADDRESS 0xB8000   /* Video memory begins here. */




void clear_screen(void);

int main(int argc, char **argv);

#endif // KERNEL_H
