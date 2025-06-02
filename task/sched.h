#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>
#include <stddef.h>
typedef enum {
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_EXITED,
    THREAD_DEAD
} thread_state_t;

typedef struct list_node {
    struct list_node* next;
    struct list_node* prev;
} list_node_t;

typedef struct {
    list_node_t* head;
    list_node_t* tail;
    size_t count;
} thread_list_t;

typedef struct thread {
    uint32_t id;
    thread_state_t state;
    uintptr_t esp;
    uintptr_t base_esp;
    list_node_t node;
} thread_t;

extern thread_t* current_thread;

// Core scheduler functions
void scheduler_init(void);
void schedule(void);
void yield(void);
void thread_enqueue(thread_t* thread);
void thread_dequeue(thread_t* thread);


#endif // SCHED_H
