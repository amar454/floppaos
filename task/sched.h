#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>
#include <stddef.h>
#include "sync/spinlock.h"
#include "../mem/alloc.h"
#include "../mem/pmm.h"
#include "../mem/vmm.h"
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
    uint32_t base_priority;
    uint32_t effective_prio;
    uint64_t ready_since;
    uint32_t run_count;
    uint32_t time_slice;
    uint64_t last_run;
    uint64_t wake_time;
    vmm_region_t* region;   
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
typedef struct core_sched {
    thread_list_t *ready_queues;  /* priority 0 = highest */
    thread_t* current;
    thread_t* idle;
} core_sched_t;
extern scheduler_t sched;
extern thread_list_t ready_queue[MAX_CORES];
extern thread_t* current_thread[MAX_CORES];
extern thread_t* idle_thread[MAX_CORES];
void sched_init(uint32_t core_count);
thread_t* sched_create_thread(void (*entry)(void), uint32_t priority);
void sched_add_thread(thread_t* t);
void sched_remove_thread(thread_t* t);

void sched_sleep(uint64_t ms);
void sched_wake(thread_t* t);

void sched_reschedule(uint32_t core_id);
void sched_timer_tick(void);

#endif
