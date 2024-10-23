/*

boot.c - code for the bootloader (16-bit real mode)

Amar Djulovic 

*/ 

#include <stdarg.h>

#define VIDEO_MEMORY 0xB8000
#define WHITE_ON_BLACK 0x0F
#define MAX_INPUT_SIZE 80

unsigned short* video_memory = (unsigned short*)VIDEO_MEMORY;
int cursor_position = 0;  // To keep track of where to print on the screen

// Function to move the cursor position in VGA text mode
void move_cursor(int position) {
    // cursor position is updated by writing to VGA ports 0x3D4 and 0x3D5
    asm volatile ("mov al, 0x0F; mov dx, 0x3D4; out dx, al; mov al, byte [bp + position]; out dx, al" :: "bp"(position));
}

// Function to clear the screen
void clear_screen() {
    for (int i = 0; i < 80 * 25; i++) {
        video_memory[i] = (WHITE_ON_BLACK << 8) | ' ';
    }
    cursor_position = 0;
    move_cursor(cursor_position);
}

// Function to output a character to the screen at the current cursor position
void put_char(char c) {
    if (c == '\n') {
        cursor_position = (cursor_position / 80 + 1) * 80;  // Move to the next line
    } else {
        video_memory[cursor_position] = (WHITE_ON_BLACK << 8) | c;
        cursor_position++;
    }
    move_cursor(cursor_position);  // Update cursor position
}

// Function to print a string
void printf(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        put_char(str[i]);
    }
}

// Function to read a single character from keyboard using BIOS interrupt 0x16
char getchar() {
    char c;
    asm volatile (
        "mov ah, 0x00;"         // BIOS function to read keyboard input
        "int 0x16;"             // Call BIOS interrupt 0x16
        "mov %0, al;"           // Store the result in variable c
        : "=r"(c)               // Output
        :                       // No input
        : "ah", "al"            // Clobbered registers
    );
    return c;
}

// Function to handle full string input from the user
void read_input(char* buffer, int max_length) {
    int i = 0;
    char c;

    while (i < max_length - 1) {
        c = getchar();  // Read a single character

        if (c == '\r') {  // Enter key (carriage return)
            put_char('\n');  // Move to the next line on the screen
            break;
        } else if (c == '\b') {  // Backspace
            if (i > 0) {
                i--;  // Move the buffer index back
                cursor_position--;  // Move the cursor back
                put_char(' ');  // Erase the character on the screen
                cursor_position--;  // Move the cursor back again
                move_cursor(cursor_position);  // Update cursor position
            }
        } else if (c >= ' ' && c <= '~') {  // Only printable characters
            buffer[i++] = c;  // Add the character to the buffer
            put_char(c);  // Display the character on the screen
        }
    }

    buffer[i] = '\0';  // Null-terminate the string
}

// Main function for the bootloader
void boot_main() {
    clear_screen();
    printf("floppaOS - fshell: ");

    char buffer[MAX_INPUT_SIZE];  // Buffer to store user input
    read_input(buffer, MAX_INPUT_SIZE);  // Read the user's input

    printf("\ncommand: ");
    printf(buffer);

    // Infinite loop to halt execution
    while (1) {}
}
