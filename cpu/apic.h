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