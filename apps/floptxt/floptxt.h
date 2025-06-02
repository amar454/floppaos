#ifndef FLOPTXT_H
#define FLOPTXT_H 
#include "../../drivers/vga/vgahandler.h"
#define MAX_FILE_SIZE 1024
#define MAX_TEXT_BUFFER_SIZE (VGA_WIDTH * VGA_HEIGHT)
#define VISIBLE_LINES (VGA_HEIGHT - 2)
void floptxt_task(void *arg);

#endif