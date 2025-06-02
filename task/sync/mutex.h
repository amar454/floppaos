
#include "spinlock.h"
#include <stdint.h>

#define SPIN_COUNT 1000 // Number of spins before blocking

typedef enum {
    MUTEX_STATE_UNLOCKED,
    MUTEX_STATE_LOCKED,
    MUTEX_STATE_CONTESTED
} mutex_state_t;

typedef struct {
    volatile mutex_state_t state; // Mutex state
    //spinlock_t lock;              // Spinlock for protecting the wait queue
    //task_queue_t wait_queue;         // Queue of tasks waiting for the mutex
} mutex_t;