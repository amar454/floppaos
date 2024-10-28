#ifndef KEYBOARD_H
#define KEYBOARD_H

// Converts a keyboard scan code to an ASCII character
char key_to_char(unsigned char key);

// Reads a key scan code from the keyboard
unsigned char read_key(void);

#endif // KEYBOARD_H
