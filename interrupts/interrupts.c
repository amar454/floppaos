#include "interrupts.h"
#include <stdint.h>
#include "../drivers/io/io.h"  // For inb and outb functions

// IDT (Interrupt Descriptor Table) declaration
struct interrupt_descriptor idt[NUM_INTERRUPTS];

// Function to set an IDT gate (entry)
void set_idt_gate(int n, uint32_t handler) {
    idt[n].offset_low = handler & 0xFFFF;
    idt[n].offset_high = (handler >> 16) & 0xFFFF;
    idt[n].selector = 0x08;  // Code segment (Kernel code segment)
    idt[n].zero = 0;
    idt[n].type_attr = 0x8E;  // Interrupt gate, present, ring 0 (kernel)
}

// Function to initialize the IDT (Interrupt Descriptor Table)
void init_idt(void) {
    // Initialize all IDT entries to zero
    for (int i = 0; i < NUM_INTERRUPTS; i++) {
        set_idt_gate(i, (uint32_t)isr_handler);  // Default handler for all interrupts
    }
    
    // Load the IDT
    uint32_t idt_ptr = (uint32_t)&idt;
    idt_flush(idt_ptr);
}

// Function to load the IDT register with the address of the IDT
void idt_flush(uint32_t idt_ptr) {
    __asm__ volatile("lidt (%0)" : : "r"(idt_ptr));
}

// Interrupt Service Routine Handler (for all interrupts)
void isr_handler(void) {
    // For now, just acknowledge that the interrupt has been handled
    // You can customize this later for specific interrupts (e.g., keyboard, PIT)
    // Print a simple message, log the interrupt, etc.
}

// Assembly function to load the IDT into the processor
void load_idt(void) {
    __asm__ volatile("lidt (%0)" : : "r"(&idt));
}

