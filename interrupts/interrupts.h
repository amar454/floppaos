#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>
extern volatile int schedule_needed;
// Define the structure for the IDT entry
typedef struct {
    uint16_t base_low;       // Lower 16 bits of the address to jump to when this interrupt is called
    uint16_t sel;            // Kernel segment selector
    uint8_t always0;         // This must always be 0
    uint8_t flags;           // Flags for the interrupt gate
    uint16_t base_high;      // Upper 16 bits of the address to jump to
} __attribute__((packed)) idt_entry_t;

// Define the structure for the IDT pointer (used for loading the IDT)
typedef struct {
    uint16_t limit;          // Size of the IDT (number of bytes minus 1)
    uint32_t base;           // Base address of the IDT
} __attribute__((packed)) idt_ptr_t;

// Define a structure for the interrupt frame (used in ISR handling)
typedef struct {
    uint32_t ds;             // Data segment
    uint32_t edi, esi, ebx, edx, ecx, eax;  // Pushed registers
    uint32_t int_no, err_code;  // Interrupt number and error code (if applicable)
    uint32_t eip, cs, eflags, useresp, ss;  // Registers pushed by the interrupt

    // Optional segment registers for saving the context during interrupts
    uint32_t es, fs, gs;    // Extra, FS, and GS segment registers (optional but common)
} interrupt_frame_t;

void init_pic();
void init_pit();
void init_idt();
void init_interrupts();
void __attribute__((naked)) pit_isr();

#endif
