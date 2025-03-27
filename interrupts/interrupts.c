// IDT and IDT pointer
#include "interrupts.h"
#include "../task/task_handler.h"
#include "../drivers/io/io.h"
#include "../drivers/vga/vgahandler.h"
#include "../lib/logging.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../mem/vmm.h"
#include "../mem/pmm.h"
#include "../mem/paging.h"
#include "../mem/utils.h"
#include "../kernel.h"

idt_entry_t idt[IDT_SIZE];
idt_ptr_t idtp;

void init_stack() { 
    uint32_t stack_top = (uint32_t)(interrupt_stack + ISR_STACK_SIZE);
    __asm__ volatile("mov %0, %%esp" :: "r"(stack_top));  // Set ESP to the top of the stack
}

// PIT ISR (IRQ0)
void __attribute__((interrupt, no_caller_saved_registers)) pit_isr(void *frame) {
    (void)frame; // Unused
    
    // Call the scheduler on each timer tick if enabled
    if (scheduler_enabled) {
        scheduler();
    }
    
    outb(PIC1_COMMAND, 0x20); // Send EOI to PIC
}

// Keyboard ISR (IRQ1)
void __attribute__((interrupt, no_caller_saved_registers)) keyboard_isr(void *frame) {
    (void)frame;  // Unused
}

// Set an IDT entry
void set_idt_entry(int n, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[n].base_low = base & 0xFFFF;
    idt[n].base_high = (base >> 16) & 0xFFFF;
    idt[n].sel = sel;
    idt[n].always0 = 0;
    idt[n].flags = flags;
}

// Initialize the Programmable Interrupt Controller (PIC)
void init_pic() {
    outb(PIC1_COMMAND, 0x11);  // Start initialization sequence
    outb(PIC2_COMMAND, 0x11);

    outb(PIC1_DATA, 0x20);  // Set PIC1 vector offset
    outb(PIC2_DATA, 0x28);  // Set PIC2 vector offset

    outb(PIC1_DATA, 0x04);  // Tell PIC1 about PIC2
    outb(PIC2_DATA, 0x02);  // Tell PIC2 its cascade identity

    outb(PIC1_DATA, 0x01);  // 8086 mode
    outb(PIC2_DATA, 0x01);

    // Enable keyboard (IRQ1) and PIT (IRQ0)
    outb(PIC1_DATA, 0xFC);  // 0b11111100 (Enable IRQ0 and IRQ1)
    outb(PIC2_DATA, 0xFF);  // Disable all IRQs on PIC2
}

// Initialize the PIT (Programmable Interval Timer)
void init_pit() {
    uint16_t divisor = 1193182 / PIT_FREQUENCY;
    outb(0x43, 0x36);  // PIT mode (Channel 0, rate generator)
    outb(0x40, divisor & 0xFF);        // Low byte
    outb(0x40, (divisor >> 8) & 0xFF); // High byte
}

// Initialize the IDT
void init_idt() {
    idtp.limit = (sizeof(idt_entry_t) * IDT_SIZE) - 1;
    idtp.base = (uint32_t)(uintptr_t)&idt;

    for (int i = 0; i < IDT_SIZE; i++) {
        set_idt_entry(i, 0, 0, 0);
    }

    log_step("Setting up IDT entries...\n", LIGHT_GRAY);

    // Set up ISR for PIT (IRQ0) and keyboard (IRQ1)
    set_idt_entry(32, (uint32_t)(uintptr_t)&pit_isr, KERNEL_CODE_SEGMENT, 0x8E);
    set_idt_entry(33, (uint32_t)(uintptr_t)&keyboard_isr, KERNEL_CODE_SEGMENT, 0x8E);

    // Load the IDT
    __asm__ volatile ("lidt (%0)" :: "r"(&idtp) : "memory");
}

// Initialize interrupts
void init_interrupts() {
    log_step("Initializing interrupts...\n", LIGHT_GRAY);

    log_step("Setting up interrupt stack...\n", LIGHT_GRAY);
    init_stack();  // Set up the interrupt stack
    log_step("Initializing PIC...\n", LIGHT_GRAY);
    init_pic();
    log_step("Initializing PIT...\n", LIGHT_GRAY);
    init_pit();
    log_step("Initializing IDT...\n", LIGHT_GRAY);
    init_idt(); 
    log_step("Interrupts initialized.\n", GREEN);
}
