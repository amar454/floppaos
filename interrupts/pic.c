#include "pic.h"
#include "../drivers/io/io.h"  // for outb function

void init_pic(void) {
    // Send the initialization command to PIC1
    outb(ICW1_INIT + ICW1_ICW4, PIC1_CMD);
    outb(ICW1_INIT + ICW1_ICW4, PIC2_CMD);

    // Set the base interrupt vectors for PIC1 and PIC2
    outb(0x20, PIC1_DATA);  // IRQ0 -> 0x20
    outb(0x28, PIC2_DATA);  // IRQ8 -> 0x28

    // Configure cascade mode and set other options
    outb(0x04, PIC1_DATA);  // Tell PIC1 about PIC2
    outb(0x02, PIC2_DATA);  // Tell PIC2 its cascade identity

    // Set up ICW4 (4-byte initialization)
    outb(0x01, PIC1_DATA);
    outb(0x01, PIC2_DATA);

    // Enable IRQs (unmask IRQs you want to listen to)
    outb(0xFF, PIC1_DATA);  // Mask all IRQs initially
    outb(0xFF, PIC2_DATA);  // Mask all IRQs initially
}
void pit_handler(void) {
    // Handle PIT interrupt (e.g., increment tick count, time management)
    // Send an EOI (End Of Interrupt) to PICs
    outb(0x20, PIC1_CMD);  // Send EOI to PIC1
    outb(0x20, PIC2_CMD);  // Send EOI to PIC2 if necessary
}

void init_pit() {
    // Set PIT frequency to 1000Hz (1ms interval)
    uint32_t divisor = 1193180 / 1000;  // Calculate the divisor for 1ms

    // Send the command byte to PIT
    outb(0x36, 0x43);  // Command to set PIT mode

    // Set the low and high bytes for the divisor
    outb(divisor & 0xFF, 0x40);  // Send low byte to channel 0
    outb((divisor >> 8) & 0xFF, 0x40);  // Send high byte to channel 0
}

