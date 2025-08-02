#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>
#include <stddef.h>

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


typedef struct thread {
    cpu_ctx_t ctx;
    thread_state_t state;
    uint8_t* stack;
    uint32_t wake_tick;
    struct thread* prev;
    struct thread* next;
    uint32_t id;
} thread_t;

extern thread_t* thread_list_head;
extern thread_t* current_thread;
extern uint32_t next_thread_id;

void sched_init(void);
thread_t* sched_create_thread(void (*entry)(void));
void sched_add_thread(thread_t* t);
void sched_remove_thread(thread_t* t);
void sched_yield(void);
void sched_sleep(uint32_t ticks);
void sched_exit(void);
void sched_tick(void);
thread_t* sched_get_current(void);
void sched_create_idle_thread(void);
void sched_start(void);

#endif // SCHED_H
