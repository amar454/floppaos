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
    struct thread* prev;
    struct thread* next;
    uint32_t id;
    uint32_t core_id; 
} thread_t;

typedef struct thread_list {
    thread_t* head;
} thread_list_t;

void sched_init(void);
thread_t* sched_create_thread(void (*entry)(void));
void sched_add_thread(thread_t* t);
void sched_remove_thread(thread_t* t);
void sched_yield(void);
void sched_exit(void);
thread_t* sched_get_current(void);
void sched_start(void);

#endif // SCHED_H
