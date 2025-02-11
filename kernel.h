#ifndef KERNEL_H
#define KERNEL_H

#include "multiboot/multiboot.h"
#include <stdint.h>

int kmain(uint32_t magic, multiboot_info_t *mb_info);
void panic(uint32_t address, const char* msg, const char* err);
#endif // KERNEL_H
