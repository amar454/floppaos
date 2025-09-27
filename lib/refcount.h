#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdatomic.h>
#include "logging.h"
typedef atomic_uint refcount_t;

static inline void refcount_init(refcount_t *references) {
    atomic_init(references, 1);
}

static inline bool refcount_inc_not_zero(refcount_t *references) {
    for (;;) {
        unsigned int old = atomic_load(references);
        if (old == 0)
            return false;

        unsigned int expected = old;
        if (atomic_compare_exchange_weak(references, 
                                         &expected, 
                                         old + 1)) return true;
            

        old = expected;
    }
}

static inline bool refcount_dec_and_test(refcount_t *references) {
    for (;;) {
        unsigned int old = atomic_load(references);
        if (old == 0) {
            log("possible uaf detected: refcount is already 0\n", RED);
            return false;
        }

        unsigned int expected = old;
        if (atomic_compare_exchange_weak(references, &expected, old - 1))
            return (old - 1) == 0;

        old = expected;
    }
}