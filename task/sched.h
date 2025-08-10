#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>
#include <stddef.h>
#include "sync/spinlock.h"
typedef struct cpu_ctx {
    uint32_t edi, esi, ebp, esp;
    uint32_t ebx, edx, ecx, eax;
    uint32_t es;
    uint32_t int_no, err;
    uint32_t eip, cs, gs, ds, fs;
    uint32_t eflags, user_esp, ss;
} cpu_ctx_t;

typedef enum thread_state {
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_EXITED,
    THREAD_DEAD,
    THREAD_SLEEPING
} thread_state_t;

#define THREAD_STACK_SIZE 4096
#define MAX_CORES 8
#define MAX_THREADS 256

typedef struct thread {
    cpu_ctx_t ctx;
    thread_state_t state;
    uint8_t* stack;
    struct thread* prev;
    struct thread* next;
    uint32_t id;
    uint32_t core_id;
    uint32_t priority;
    uint32_t time_slice;
    uint32_t cpu_usage;
    uint64_t last_run;
    uint64_t wake_time;
} thread_t;

typedef struct thread_list {
    thread_t* head;
    thread_t* tail;
    uint32_t count;
} thread_list_t;

typedef struct scheduler {
    thread_list_t ready_queues[MAX_CORES];
    thread_list_t sleep_queue;
    thread_t* current_threads[MAX_CORES];
    thread_t* idle_threads[MAX_CORES];
    uint32_t core_count;
    uint32_t thread_count;
    uint32_t next_tid;
    spinlock_t sched_lock;
} scheduler_t;

extern scheduler_t sched;

void sched_init(void);
thread_t* sched_create_thread(void (*entry)(void), uint32_t priority);
void sched_add_thread(thread_t* t);
void sched_remove_thread(thread_t* t);
void sched_yield(void);
void sched_exit(void);
thread_t* sched_get_current(void);
void sched_start(void);
void sched_sleep(uint64_t ms);
void sched_wake(thread_t* t);
uint32_t sched_get_core_id(void);
void sched_reschedule(uint32_t core_id);
void sched_balance_load(void);
void sched_timer_tick(void);

#endif
