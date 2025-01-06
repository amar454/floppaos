#include "ps2ms.h"
#include "../vga/vgahandler.h"
#include "../io/io.h"
#include <stdint.h>

// Mouse state
static int mouse_x = 0;
static int mouse_y = 0;
static int left_button_pressed = 0;
static int right_button_pressed = 0;
static int middle_button_pressed = 0;

// Mouse packet structure
static struct mouse_packet {
    uint8_t button_status;
    int8_t x_offset;
    int8_t y_offset;
    int8_t scroll_offset;
} packet;

// Non-blocking function to read mouse data
uint8_t try_read_mouse_data(void) {
    if (inb(0x64) & 0x1) {
        return inb(0x60);  // Return data byte if available
    }
    return 0;  // No mouse data available
}

// Polling function to read mouse data (without IRQ)
void poll_mouse(void) {
    static int byte_count = 0;
    static uint8_t mouse_data[3];

    uint8_t data_byte = try_read_mouse_data();
    if (data_byte != 0) {
        mouse_data[byte_count] = data_byte;
        byte_count++;

        // If we have received 3 bytes of data, process them
        if (byte_count == 3) {
            packet.button_status = mouse_data[0];
            packet.x_offset = (int8_t)mouse_data[1];
            packet.y_offset = (int8_t)mouse_data[2];

            // Extract the button status
            left_button_pressed = packet.button_status & 0x1;
            right_button_pressed = (packet.button_status >> 1) & 0x1;
            middle_button_pressed = (packet.button_status >> 2) & 0x1;

            // Update mouse position
            mouse_x += packet.x_offset;
            mouse_y -= packet.y_offset;  // Y-axis is inverted

            // Ensure the mouse stays within the screen bounds
            if (mouse_x < 0) mouse_x = 0;
            if (mouse_x >= VGA_WIDTH) mouse_x = VGA_WIDTH - 1;
            if (mouse_y < 0) mouse_y = 0;
            if (mouse_y >= VGA_HEIGHT) mouse_y = VGA_HEIGHT - 1;

            // Reset byte count
            byte_count = 0;
        }
    }
}

// Non-blocking function to get mouse position
int mouse_get_position(int *x, int *y) {
    *x = mouse_x;
    *y = mouse_y;
    return 1;
}

// Non-blocking function to check mouse button status
int mouse_get_button_status(int button) {
    switch (button) {
        case 0: return left_button_pressed;    // Left button
        case 1: return right_button_pressed;   // Right button
        case 2: return middle_button_pressed;  // Middle button
        default: return 0; // Invalid button
    }
}

// Mouse task to handle and process mouse events (position and button clicks)
void mouse_task(void *arg) {
    int x, y;

    // Poll the mouse to get updated data
    poll_mouse();

    // Get the current mouse position
    if (mouse_get_position(&x, &y)) {
        // You can process the mouse movement here, e.g., draw the cursor
        //vga_plot_pixel(x, y, WHITE);  // Draw a simple 'O' as the cursor
    }

    // Check if a button was clicked
    if (mouse_get_button_status(0)) {
        // Left button pressed - perform some action
        //console_print("Left Button Clicked\n");
    }
    if (mouse_get_button_status(1)) {
        // Right button pressed - perform another action
        //console_print("Right Button Clicked\n");
    }
    if (mouse_get_button_status(2)) {
        // Middle button pressed - perform a different action
        //console_print("Middle Button Clicked\n");
    }
}

// Initialize the mouse (send the command to the mouse controller to enable it)
void mouse_init() {
    // Enable mouse reporting
    outb(0x64, 0xA8);  // Enable mouse reporting
    outb(0x64, 0x20);  // Get current state
    uint8_t status = inb(0x60);
    outb(0x64, 0x60);  // Set the state
    outb(0x60, status | 2);  // Enable mouse interrupts

    // Send initialization command to the mouse
    outb(0x64, 0xD4);
    outb(0x60, 0xF4);  // Enable the mouse
}
