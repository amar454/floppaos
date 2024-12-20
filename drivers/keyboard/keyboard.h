#ifndef KEYBOARD_H
#define KEYBOARD_H
#include <stdbool.h>

// Arrow keys
#define ARROW_UP             0x48
#define ARROW_DOWN           0x50
#define ARROW_LEFT           0x4B
#define ARROW_RIGHT          0x4D

// Modifier keys
#define KEY_SHIFT_PRESS      0x2A
#define KEY_SHIFT_RELEASE    0xAA
#define KEY_CTRL_PRESS       0x1D
#define KEY_CTRL_RELEASE     0x9D
#define KEY_ALT_PRESS        0x38
#define KEY_ALT_RELEASE      0xB8

// Special keys
#define KEY_BACKSPACE        0x0E
#define KEY_ENTER            0x1C
#define KEY_SPACE            0x39

// Extended key prefix
#define KEY_EXTENDED         0xE0

// Converts a keyboard scan code to an ASCII character
void keyboard_task(void *arg);
char key_to_char(unsigned char key);

// Reads a key scan code from the keyboard
unsigned char try_read_key(void);
char get_char(void);
char try_get_char(void);
#endif // KEYBOARD_H
