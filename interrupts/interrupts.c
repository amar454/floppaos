#include "interrupts.h"
#include "../task/task_handler.h"
#include "../drivers/io/io.h"

#define IDT_SIZE 256
#define KERNEL_CODE_SEGMENT 0x08
#define PIT_FREQUENCY 1000  // 1ms interval (1000 Hz)

// IDT and PIC registers
idt_entry_t idt[IDT_SIZE];
idt_ptr_t idtp;

// Define the PIC commands and data ports
#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

// Initialize the PIC (Programmable Interrupt Controller)
void init_pic() {
    outb(PIC1_COMMAND, 0x11);  // Start initialization sequence
    outb(PIC2_COMMAND, 0x11);

    outb(PIC1_DATA, 0x20);     // Master offset to 0x20
    outb(PIC2_DATA, 0x28);     // Slave offset to 0x28

    outb(PIC1_DATA, 0x04);     // Tell PIC1 that there is a slave at IRQ2
    outb(PIC2_DATA, 0x02);     // Tell PIC2 its cascade identity

    outb(PIC1_DATA, 0x01);     // Set PICs to 8086 mode
    outb(PIC2_DATA, 0x01);

    outb(PIC1_DATA, 0x00);     // Enable interrupts on PIC1
    outb(PIC2_DATA, 0x00);     // Enable interrupts on PIC2
}

// Initialize the PIT (Programmable Interval Timer)
void init_pit() {
    uint16_t divisor = 1193182 / PIT_FREQUENCY;  // 1ms interval (1000 Hz)

    outb(0x43, 0x36);  // Set PIT mode (Channel 0, rate generator)
    outb(0x40, divisor & 0xFF);        // Low byte
    outb(0x40, (divisor >> 8) & 0xFF); // High byte
}

// Function to set an IDT entry
void set_idt_entry(int n, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[n].base_low = base & 0xFFFF;
    idt[n].base_high = (base >> 16) & 0xFFFF;
    idt[n].sel = sel;
    idt[n].always0 = 0;
    idt[n].flags = flags;
}

// Initialize the IDT (Interrupt Descriptor Table)
void init_idt() {
    idtp.limit = (sizeof(idt_entry_t) * IDT_SIZE) - 1;
    idtp.base = (uint32_t)&idt;

    // Zero out the IDT
    for (int i = 0; i < IDT_SIZE; i++) {
        set_idt_entry(i, 0, 0, 0);
    }

    // Set up PIT ISR (IRQ0)
    set_idt_entry(32, (uint32_t)&pit_isr, KERNEL_CODE_SEGMENT, 0x8E);

    // Load the IDT
    __asm__ volatile (
        "lidt (%0)"  // Load IDT pointer into the IDTR register
        :
        : "r"(&idtp)
        : "memory"
    );
}

void __attribute__((interrupt)) pit_isr(interrupt_frame_t *frame) {
    // Manually save registers to ensure safe calling of scheduler
    __asm__ volatile (
        "pusha\n\t"               // Push all general-purpose registers to stack
        "push %ds\n\t"            // Push DS (data segment)
        "push %es\n\t"            // Push ES (extra segment)
        "push %fs\n\t"            // Push FS (segment)
        "push %gs\n\t"            // Push GS (segment)

        // Set up the data segments for the ISR (kernel DS)
        "mov $0x10, %ax\n\t"      // Load the data segment (kernel DS)
        "mov %ax, %ds\n\t"        // Set DS
        "mov %ax, %es\n\t"        // Set ES
        "mov %ax, %fs\n\t"        // Set FS
        "mov %ax, %gs\n\t"        // Set GS
    );

    // Call the task scheduler function (no SSE instructions here)
    scheduler();

    // End of interrupt (EOI) handling for PIC1 and PIC2
    __asm__ volatile (
        "mov $0x20, %al\n\t"      // EOI command for PIC1
        "outb %al, $0x20\n\t"     // Send EOI to PIC1
        "mov $0x20, %al\n\t"      // EOI command for PIC2
        "outb %al, $0xA0\n\t"     // Send EOI to PIC2
    );

    // Restore registers manually
    __asm__ volatile (
        "pop %gs\n\t"             // Restore GS
        "pop %fs\n\t"             // Restore FS
        "pop %es\n\t"             // Restore ES
        "pop %ds\n\t"             // Restore DS
        "popa\n\t"                // Restore all general-purpose registers
        "iret\n\t"                // Return from interrupt
    );
}

