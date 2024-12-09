#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>
#include "../drivers/io/io.h"  // For inb and outb functions

// Interrupt Gate Descriptor Structure
struct interrupt_descriptor {
    uint16_t offset_low;   // Lower 16 bits of the ISR address
    uint16_t selector;     // Code segment selector
    uint8_t zero;          // Reserved, should be 0
    uint8_t type_attr;     // Type and attributes (e.g., present, privilege level)
    uint16_t offset_high;  // Higher 16 bits of the ISR address
};

// Define the number of interrupt vectors
#define NUM_INTERRUPTS 256

// Interrupt Descriptor Table (IDT)
extern struct interrupt_descriptor idt[NUM_INTERRUPTS];

// Function prototypes
void init_idt(void);
void set_idt_gate(int n, uint32_t handler);
void idt_flush(uint32_t idt_ptr);
void isr_handler(void);

// Assembly functions
void load_idt(void);

#endif // INTERRUPTS_H
