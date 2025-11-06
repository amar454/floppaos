#ifndef SMP_H
#define SMP_H

#include <stdint.h>
#include <stdatomic.h>

#define SMP_IPI_VECTOR 0xF0

#ifndef CONFIG_MAX_CPUS
#define CONFIG_MAX_CPUS 64
#endif

/* Public API */
void smp_init_bsp(void);
int smp_register_cpu(uint8_t apic_id);
int smp_fetch_cpu(void);

int smp_cpu_count(void);

void smp_tell_other_cpus_to_do_fn(void (*fn)(void*), void* arg);

void smp_handle_ipi(void);

#endif /* SMP_H */
