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

typedef struct cpu_context {
    unsigned edi;
    unsigned esi;
    unsigned ebx;
    unsigned ebp;
    unsigned eip;
} cpu_context_t;
typedef struct per_cpu {
    unsigned char apic_id;
    cpu_context_t context;




} per_cpu_t;

