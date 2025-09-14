

#include "smp.h"
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
#include <stdatomic.h>

extern uint32_t lapic_read(uint32_t offset);
extern void lapic_write(uint32_t offset, uint32_t value);

static atomic_int cpu_count_atomic = 0; 
static atomic_int smp_initialized = 0;

static uint8_t cpu_apic_id[CONFIG_MAX_CPUS];

static void (*remote_fn[CONFIG_MAX_CPUS])(void *);
static void *remote_arg[CONFIG_MAX_CPUS];

/* pending flag: 1 == a remote_fn is pending for that cpu */
static atomic_int remote_pending[CONFIG_MAX_CPUS];

static atomic_uint_fast32_t remote_seq[CONFIG_MAX_CPUS];

int smp_fetch_cpu(void) {
    uint32_t apicid = (lapic_read(0x20) >> 24) & 0xFF;
    int count = atomic_load(&cpu_count_atomic);
    for (int i = 0; i < count; ++i) {
        if (cpu_apic_id[i] == (uint8_t)apicid) return i;
    }
    return 0;
}



int smp_cpu_count(void) {
    return atomic_load(&cpu_count_atomic);
}

static void send_ipi_to_apic(uint8_t apic_id, uint8_t vector)
{

    lapic_write(0x310, ((uint32_t)apic_id) << 24);

    lapic_write(0x300, (uint32_t)vector);

    while (lapic_read(0x300) & (1 << 12)) {
        IA32_CPU_RELAX();
    }
}

void smp_init_bsp(void)
{
    if (atomic_exchange(&smp_initialized, 1) != 0) {
        return;
    }

    uint32_t apicid = (lapic_read(0x20) >> 24) & 0xFF;
    cpu_apic_id[0] = (uint8_t)apicid;
    atomic_store(&cpu_count_atomic, 1);


    for (int i = 0; i < CONFIG_MAX_CPUS; ++i) {
        remote_fn[i] = NULL;
        remote_arg[i] = NULL;
        atomic_store(&remote_pending[i], 0);
        atomic_store(&remote_seq[i], 0);
    }

    log_uint("smp: BSP initialized, apic id: \n", apicid);
}

int smp_register_cpu(uint8_t apic_id)
{
    int id = atomic_fetch_add(&cpu_count_atomic, 1);
    if (id >= CONFIG_MAX_CPUS) {
        atomic_fetch_sub(&cpu_count_atomic, 1);
        log_uint("smp: too many CPUs registered, ignoring apic id ", apic_id);
        return -1;
    }
    cpu_apic_id[id] = apic_id;

    remote_fn[id] = NULL;
    remote_arg[id] = NULL;
    atomic_store(&remote_pending[id], 0);
    atomic_store(&remote_seq[id], 0);

    log_f("smp: CPU %u registered, apic id %u\n", id, apic_id);
    return id;
}


void smp_handle_ipi(void)
{
    int me = smp_my_cpu();
    if (me < 0 || me >= CONFIG_MAX_CPUS) return;

    if (!atomic_load(&remote_pending[me])) {
        return;
    }

    void (*fn)(void *) = remote_fn[me];
    void *arg = remote_arg[me];

    if (fn) {
        fn(arg);
    }

    remote_fn[me] = NULL;
    remote_arg[me] = NULL;
    atomic_store_explicit(&remote_pending[me], 0, memory_order_release);
    atomic_fetch_add(&remote_seq[me], 1);
}


void smp_tell_other_cpus_to_do_fn(void (*fn)(void *), void *arg)
{
    if (!fn) return;
    int me = smp_fetch_cpu();
    int num_cpus = smp_cpu_count();
    if (num_cpus <= 1) {
        return;
    }

    for (int c = 0; c < num_cpus; ++c) {
        if (c == me) continue;
        remote_fn[c] = fn;
        remote_arg[c] = arg;
        atomic_store_explicit(
            &remote_pending[c], 
            1, 
            memory_order_release);
    }

    atomic_thread_fence(memory_order_seq_cst);

    for (int c = 0; c < num_cpus; ++c) {
        if (c == me) continue;
        uint8_t apic = cpu_apic_id[c];
        send_ipi_to_apic(apic, SMP_IPI_VECTOR);
    }

    /* wait for all remotes to clear pending */
    for (int c = 0; c < num_cpus; ++c) {
        if (c == me) continue;
        int spins = 0;
        while (atomic_load_explicit(&remote_pending[c], memory_order_acquire)) {
            cpu_relax();
            spins++;
            if ((spins & 0xFFF) == 0) {
            }
        }
    }
}
