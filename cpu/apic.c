#include "../mem/alloc.h"
#include "../mem/pmm.h"
#include "../mem/vmm.h"
#include "../lib/logging.h"
#include "../lib/str.h"
#include "../interrupts/interrupts.h"
#include "../task/sync/spinlock.h"
#include "../drivers/time/floptime.h"
#include <stdint.h>
#include <stddef.h>
#define LOCAL_APIC_BASE 0xfee00000
#define LOCAL_APIC_ID_REG 0x20
#define LOCAL_APIC_VER_REG 0x30
#define LOCAL_APIC_TPR_REG 0x80
#define LOCAL_APIC_EOI 0xb0
#define LOCAL_APIC_LDR_REG 0xd0
#define LOCAL_APIC_LDF_REG 0xe0
#define LOCAL_APIC_SPURIOUS_REG 0xf0
#define LOCAL_APIC_CMCI_LVT_REG 0x2f0
#define LOCAL_APIC_ICR_LOW_REG 0x300
#define LOCAL_APIC_ICR_HIGH_REG 0x310
#define LOCAL_APIC_TIMER_LVT_REG 0x320
#define LOCAL_APIC_TM_LVT_REG 0x330
#define LOCAL_APIC_PERFC_LVT_REG 0x340
#define LOCAL_APIC_LINT0_LVT_REG 0x350
#define LOCAL_APIC_LINT1_LVT_REG 0x360
#define LOCAL_APIC_INIT_COUNT_REG 0x380
#define LOCAL_APIC_CURRENT_COUNT_REG 0x390
#define LOCAL_APIC_ERR_LVT_REG 0x370
#define LOCAL_APIC_DCR_REG 0x3e0
#define IO_APIC_IND     0x0
#define IO_APIC_DATA    0x10
#define APIC_IND_ID     0x0
#define APIC_IND_VER    0x1
#define APIC_IND_REDIR  0x10
#define APIC_LVT_MASK (1 << 16)
#define APIC_LVT_VECTOR 0x1
#define APIC_LVT_DELIVERY_MODE_FIXED 0
#define APIC_LVT_TIMER_MODE_ONE_SHOT 0
#define APIC_LVT_TIMER_MODE_PERIODIC (1 << 17)
#define APIC_POLARITY_ACTIVE_LOW 1
#define APIC_POLARITY_ACTIVE_HIGH 0
#define APIC_TRIGGER_EDGE 0
#define APIC_TRIGGER_LEVEL 1
#define IPI_INIT 0x5
#define IPI_STARTUP 0x6
#define APIC_CALIBRATE_TICKS 10
typedef struct io_apic {
    uint8_t apic_id;
    uint32_t virt_base_address;          
    uint32_t phys_base_address;
    struct io_apic* next;
    struct io_apic* prev;
} io_apic_t;


static uint32_t lapic_base = 0;

static uint32_t lapic_read(uint32_t offset) { 
    uint32_t* reg;
    reg = (uint32_t*)(lapic_base + offset);
    return *reg;
}
static void lapic_write(uint32_t offset, uint32_t value) { 
    uint32_t* reg;
    reg = (uint32_t*)(lapic_base + offset);
    *reg = value;
}

static void init_local_apic() { 
    uint32_t tmp;
    lapic_write(LOCAL_APIC_LDR_REG, 0x0);
    tmp = (1 >> smp_fetch_cpu());
    tmp = tmp << 24;
    lapic_write(LOCAL_APIC_LDR_REG, 0xffffffff);
    tmp = lapic_read(LOCAL_APIC_SPURIOUS_REG);
    tmp = tmp | (1 << 8);
    lapic_write(LOCAL_APIC_SPURIOUS_REG, tmp);
}

void apic_bsp_init(uint32_t phys_base_address) {
    if (lapic_base) return;
    lapic_base = phys_base_address;
    init_local_apic();
}
#define UINT_MAX 4294967295
static uint32_t ticks[8];
void timer_wait_ticks(uint32_t t) {
    int wait_idle = 0;
    uint32_t current_ticks;
    uint32_t eflags = get_eflags();
    if (IRQ_ENABLED(eflags))
        wait_idle = 1;
    current_ticks = ticks[0];
    while (ticks[0] < current_ticks + t) {
            if (wait_idle)
                asm("hlt");
            else
                asm("pause");
    }
}

void apic_init_timer(int vector) {
    uint32_t timer_lvt;
    uint32_t apic_ticks;
    uint32_t apic_ticks_per_second;
    timer_lvt = APIC_LVT_DELIVERY_MODE_FIXED + APIC_LVT_MASK + APIC_LVT_TIMER_MODE_ONE_SHOT + APIC_LVT_VECTOR*vector;
    lapic_write(LOCAL_APIC_TIMER_LVT_REG, timer_lvt);
    lapic_write(LOCAL_APIC_DCR_REG, 0xa);
    lapic_write(LOCAL_APIC_INIT_COUNT_REG, UINT_MAX);
    timer_wait_ticks(APIC_CALIBRATE_TICKS);
    apic_ticks = lapic_read(LOCAL_APIC_CURRENT_COUNT_REG);
    apic_ticks = ~apic_ticks;
    apic_ticks_per_second = apic_ticks * 100 / APIC_CALIBRATE_TICKS;

    timer_lvt = APIC_LVT_DELIVERY_MODE_FIXED +  APIC_LVT_TIMER_MODE_PERIODIC + APIC_LVT_VECTOR*vector;
    lapic_write(LOCAL_APIC_TIMER_LVT_REG, timer_lvt);

    lapic_write(LOCAL_APIC_INIT_COUNT_REG, apic_ticks / APIC_CALIBRATE_TICKS);
}