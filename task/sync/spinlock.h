#ifndef SPINLOCK_H
#define SPINLOCK_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>
#include "../../interrupts/interrupts.h"
#define SPINLOCK_INIT {ATOMIC_VAR_INIT(0)}
typedef struct spinlock {
    atomic_uint state;
} spinlock_t;

static inline void spinlock_init(spinlock_t* lock) {
    atomic_store(&lock->state, 0);
}

static inline bool spinlock_trylock(spinlock_t* lock) {
    unsigned int expected = 0;
    return __atomic_compare_exchange_n(
        &lock->state, 
        &expected, 
        1,
        false,
        __ATOMIC_ACQUIRE, 
        __ATOMIC_RELAXED
    );
}

static inline bool spinlock(spinlock_t* lock) {
    bool interrupts_enabled = IA32_INT_ENABLED();
    __asm__ volatile ("cli");
    while (!spinlock_trylock(lock)) {
        IA32_CPU_RELAX();
    }
    return interrupts_enabled;
}

static inline void spinlock_unlock(spinlock_t* lock, bool restore_interrupts) {
    __atomic_clear(&lock->state, __ATOMIC_RELEASE);
    if (restore_interrupts) {
        __asm__ volatile ("sti");
    }
}

static inline void spinlock_noint(spinlock_t* lock) {
    while (__atomic_test_and_set(&lock->state, __ATOMIC_ACQUIRE)) {
        IA32_CPU_RELAX();
    }
}

static inline void spinlock_unlock_noint(spinlock_t* lock) {
    __atomic_clear(&lock->state, __ATOMIC_RELEASE);
}
#endif // SPINLOCK_H