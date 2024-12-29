#include "interrupts.h"
#include "../task/task_handler.h"
#include "../drivers/io/io.h"
#include "../drivers/vga/vgahandler.h"
#include "../apps/echo.h"
#define IDT_SIZE 256
#define KERNEL_CODE_SEGMENT 0x08
#define PIT_FREQUENCY 1000  // 1ms interval (1000 Hz)
volatile int schedule_needed = 0; // Flag to signal that a context switch is required

// IDT and PIC registers
idt_entry_t idt[IDT_SIZE];
idt_ptr_t idtp;

// PIC Ports
#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1
#define ISR_STACK_SIZE 8192
char interrupt_stack[ISR_STACK_SIZE];

void init_stack() {
    uint32_t stack_top = (uint32_t)(interrupt_stack + ISR_STACK_SIZE);
    asm volatile("mov %0, %%esp" :: "r"(stack_top));  // Set the stack pointer to the top of the interrupt stack
}
// Initialize the PIC (Programmable Interrupt Controller)
void init_pic() {
    // Send initialization command to PIC1 and PIC2
    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);

    // Set interrupt vectors
    outb(PIC1_DATA, 0x20);  // Master PIC offset
    outb(PIC2_DATA, 0x28);  // Slave PIC offset

    // Configure master/slave relationship
    outb(PIC1_DATA, 0x04);  // Tell PIC1 that PIC2 is at IRQ2
    outb(PIC2_DATA, 0x02);  // Tell PIC2 its cascade identity

    // Set PICs to 8086 mode
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    // Enable IRQs on both PICs
    outb(PIC1_DATA, 0x00);  // Enable all IRQs on PIC1
    outb(PIC2_DATA, 0x00);  // Enable all IRQs on PIC2
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


// Helper function to set the schedule_needed flag
void set_schedule_flag() {
    schedule_needed = 1;  // Set the flag before the interrupt
    pit_isr();  // Call the naked ISR
}

// PIT interrupt handler (ISR)
void __attribute__((naked)) pit_isr() {
    asm volatile (
        // Save registers
        "push %eax\n\t"        // Save EAX
        "push %ebx\n\t"        // Save EBX
        "push %ecx\n\t"        // Save ECX
        "push %edx\n\t"        // Save EDX
        "push %esi\n\t"        // Save ESI
        "push %edi\n\t"        // Save EDI
        "push %ebp\n\t"        // Save EBP
        "push %ds\n\t"         // Save DS
        "push %es\n\t"         // Save ES

        // Set up kernel data segment
        "mov $0x10, %ax\n\t"   // Load kernel data segment selector
        "mov %ax, %ds\n\t"     // Set DS
        "mov %ax, %es\n\t"     // Set ES

        // Signal a context switch is needed
        "mov $1, %eax\n\t"     // Set 1 (context switch required) to EAX
        "mov %eax, schedule_needed\n\t"

        // Restore registers
        "pop %es\n\t"          // Restore ES
        "pop %ds\n\t"          // Restore DS
        "pop %ebp\n\t"         // Restore EBP
        "pop %edi\n\t"         // Restore EDI
        "pop %esi\n\t"         // Restore ESI
        "pop %edx\n\t"         // Restore EDX
        "pop %ecx\n\t"         // Restore ECX
        "pop %ebx\n\t"         // Restore EBX
        "pop %eax\n\t"         // Restore EAX

        // Send End of Interrupt (EOI) to PIC
        "mov $0x20, %al\n\t"   // Load EOI command
        "outb %al, $0x20\n\t"  // Send EOI to PIC1

        // Return from interrupt
        "iret\n\t"
    );
}


// Combined Initialization
void init_interrupts() {
    //echo("Initializing interrupt stack...", unsigned char color)
    init_stack();
    //echo(const char *str, unsigned char color)
    init_pic();
    init_pit();
    init_idt();
    asm volatile("sti");  // Enable interrupts
}