#ifndef KEYBOARD_H
#define KEYBOARD_H
#include <stdbool.h>

// Converts a keyboard scan code to an ASCII character
void keyboard_task(void *arg);
char key_to_char(unsigned char key);

// Reads a key scan code from the keyboard
unsigned char read_key(void);
char get_char(void);
#endif // KEYBOARD_H
