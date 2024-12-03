#include "../../drivers/vga/vgahandler.h"
#include "../../lib/str.h"
#include <stdint.h>
#include "../echo.h"
void dsp(char *str) {
    if (flopstrcmp(str, "circle")) {
        char terminal_buffer[VGA_WIDTH * VGA_HEIGHT];
        uint8_t terminal_colors[VGA_WIDTH * VGA_HEIGHT];
        vga_initialize();  // Initialize VGA graphics mode
        
        // Draw a circle for 10 seconds
        draw_circle_for_10_seconds();

        // Switch back to text mode after the drawing is done
        vga_switch_to_text_mode();

        // Restore the terminal buffer
        restore_terminal_buffer(terminal_buffer, terminal_colors);

        
    }
}