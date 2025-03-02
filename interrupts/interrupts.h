#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>

// IDT entry structure
typedef struct {
    uint16_t base_low;
    uint16_t sel;
    uint8_t always0;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed)) idt_entry_t;

// IDT pointer structure
typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

// ISR function pointer type
typedef void (*isr_t)(void);

// Interrupt Descriptor Table (IDT) size
#define IDT_SIZE 256
#define ISR_STACK_SIZE 8192  // 8 KB Stack for Interrupts
static uint8_t interrupt_stack[ISR_STACK_SIZE] __attribute__((aligned(16)));  // Ensure stack is aligned

// Kernel code segment selector
#define KERNEL_CODE_SEGMENT 0x08

// PIC Ports
#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

// PIT Frequency (in Hz)
#define PIT_FREQUENCY 1000  // 1ms interval

// Function declarations
void init_interrupts();
void init_idt();
void init_pic();
void init_pit();
void set_idt_entry(int n, uint32_t base, uint16_t sel, uint8_t flags);

// ISR Handlers
void __attribute__((interrupt, no_caller_saved_registers)) pit_isr(void *frame);
void __attribute__((interrupt, no_caller_saved_registers)) keyboard_isr(void *frame);

#endif // INTERRUPTS_H
