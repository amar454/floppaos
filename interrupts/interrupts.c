#include "interrupts.h"
#include "../task/sched.h"
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
#include "../kernel/kernel.h"
#include <stdbool.h>

extern void isr0();
extern void isr6();
extern void isr13();
extern void isr14();

extern void irq0();
extern void irq1();

uint32_t global_tick_count = 0;
idt_entry_t idt[IDT_SIZE];
idt_ptr_t idtp;

void init_stack() {
    uint32_t stack_top = (uint32_t) (interrupt_stack + ISR_STACK_SIZE);
    __asm__ volatile("mov %0, %%esp" ::"r"(stack_top));
}

#define PIT_FREQUENCY 100

// wrapper for the scheduler tick
void scheduler_tick() {
    return;
}

extern void isr0();
extern void isr6();
extern void isr13();
extern void isr14();

extern void irq0();
extern void irq1();

/* interupt service routines */

void c_isr0(void) {
    log("isr0: divide by zero error, get fucked\n", RED);
    __asm__("hlt");
}

void c_isr6(void) {
    log("isr6: invalid opcode error, get fucked\n", RED);
    __asm__("hlt");
}

void c_isr13(void) {
    log("isr13: general protection fault, get fucked\n", RED);
    __asm__("hlt");
}

// page fault
void c_isr14(void) {
    uint32_t addr;
    uint32_t error_code;

    __asm__ volatile("mov %%cr2, %0" : "=r"(addr));

    __asm__ volatile("movl 4(%%esp), %0" : "=r"(error_code));

    log("isr14: page fault, get fucked\n", RED);
    log_address("Faulting Address: ", addr);
    log_uint("Error Code: ", error_code);
    __asm__("hlt");
}

typedef enum {
    IRQ_PIT = 0,
    IRQ_KEYBOARD = 1,
    IRQ_CASCADE = 2,
    IRQ_COM2 = 3,
    IRQ_COM1 = 4,
    IRQ_LPT2 = 5,
    IRQ_FLOPPY = 6,
    IRQ_LPT1 = 7,
    IRQ_CMOS = 8,
    IRQ_UNIMPLEMENTED0 = 9,
    IRQ_UNIMPLEMENTED1 = 10,
    IRQ_UNIMPLEMENTED2 = 11,
    IRQ_UNIMPLEMENTED3 = 12,
    IRQ_UNIMPLEMENTED4 = 13,
    IRQ_UNIMPLEMENTED5 = 14,
    IRQ_UNIMPLEMENTED6 = 15
} irq_num_t;

void _pic_register_eoi(irq_num_t irq) {
    if (irq >= IRQ_CMOS) {
        outb(PIC2_COMMAND, PIC_EOI);
    } else {
        outb(PIC1_COMMAND, PIC_EOI);
    }
}

// pit
void c_irq0(void) {
    global_tick_count++;
    scheduler_tick();
    _pic_register_eoi(IRQ_PIT);
}

// keyboard
void c_irq1(void) {
    // eh
    _pic_register_eoi(IRQ_KEYBOARD);
}

/* the following are unimplemented IRQs for now */

// cascade
void c_irq2(void) {
    _pic_register_eoi(IRQ_CASCADE);
}

// COM2
void c_irq3(void) {
    _pic_register_eoi(IRQ_COM2);
}

// COM1
void c_irq4(void) {
    _pic_register_eoi(IRQ_COM1);
}

// LPT2
void c_irq5(void) {
    _pic_register_eoi(IRQ_LPT2);
}

// floppy
void c_irq6(void) {
    _pic_register_eoi(IRQ_FLOPPY);
}

// LPT1
void c_irq7(void) {
    _pic_register_eoi(IRQ_LPT1);
}

// CMOS
void c_irq8(void) {
    _pic_register_eoi(IRQ_CMOS);
}

void set_idt_entry(int n, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[n].base_low = base & 0xFFFF;
    idt[n].base_high = (base >> 16) & 0xFFFF;
    idt[n].sel = sel;
    idt[n].always0 = 0;
    idt[n].flags = flags;
}

static void _pic_init() {
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);

    outb(PIC1_DATA, PIC1_V_OFFSET);
    outb(PIC2_DATA, PIC2_V_OFFSET);

    outb(PIC1_DATA, PIC1_IRQ2);
    outb(PIC2_DATA, PIC2_CSC_ID);

    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    outb(PIC1_DATA, PIC1_MASK);
    outb(PIC2_DATA, PIC2_MASK);

    log("pic init - ok\n", GREEN);
}

static void _pit_init() {
    uint16_t divisor = PIT_BASE_FREQUENCY / PIT_FREQUENCY;

    outb(PIT_COMMAND_PORT, PIT_COMMAND_MODE);

    outb(PIT_CHANNEL0_PORT, divisor & PIT_DIVISOR_LSB_MASK);
    outb(PIT_CHANNEL0_PORT, (divisor >> PIT_DIVISOR_MSB_SHIFT) & PIT_DIVISOR_LSB_MASK);
    log("pit init - ok\n", GREEN);
}

extern void syscall_handler();

// set idt entries for the isr and irq and load them
static void _idt_init() {
    idtp.limit = (sizeof(idt_entry_t) * IDT_SIZE) - 1;
    idtp.base = (uint32_t) &idt;

    for (size_t i = 0; i < IDT_SIZE; i++) {
        set_idt_entry(i, 0, 0, 0);
    }

    set_idt_entry(0, (uint32_t) isr0, KERNEL_CODE_SEGMENT, 0x8E);
    set_idt_entry(6, (uint32_t) isr6, KERNEL_CODE_SEGMENT, 0x8E);
    set_idt_entry(13, (uint32_t) isr13, KERNEL_CODE_SEGMENT, 0x8E);
    set_idt_entry(14, (uint32_t) isr14, KERNEL_CODE_SEGMENT, 0x8E);

    set_idt_entry(32, (uint32_t) irq0, KERNEL_CODE_SEGMENT, 0x8E);
    set_idt_entry(33, (uint32_t) irq1, KERNEL_CODE_SEGMENT, 0x8E);

    set_idt_entry(80, (uint32_t) syscall_handler, KERNEL_CODE_SEGMENT, 0x8E);

    __asm__ volatile("lidt (%0)" ::"r"(&idtp) : "memory");
    __asm__ volatile("sti");
    log("idt init - ok\n", GREEN);
}

static void disable_interrupts() {
    __asm__ volatile("cli");
}

static void enable_interrupts() {
    __asm__ volatile("sti");
}

void interrupts_init() {
    log("initializing interrupts...\n", LIGHT_GRAY);
    init_stack();
    _pic_init();
    _pit_init();
    _idt_init();
    log("interrupts init - ok.\n", GREEN);
}
