#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "sched.h"
#include "thread.h"
#include "../lib/logging.h"
#include "../mem/alloc.h"
#include "../mem/utils.h"

extern void context_switch(thread_t* new_thread);
extern void _thread_entry(void (*fn)(void*), void* arg);  // Assembly function
extern uint32_t stack_space; // from boot.asm

#define THREAD_LIST_INIT ((thread_list_t){ .count = 0, .head = NULL, .tail = NULL })
#define LIST_CONTAINER_GET(ptr, type, member) ((type*)((char*)(ptr) - offsetof(type, member)))

static thread_list_t ready_list = THREAD_LIST_INIT;
thread_t* current_thread = NULL;
static void _initial_node(thread_list_t* list, list_node_t* node) {
    log("sched: initializing thread list with single node\n", WHITE);
    node->next = node->prev = NULL;
    list->head = list->tail = node;
    list->count = 1;
}

static void _node_append(thread_list_t* list, list_node_t* pos, list_node_t* node) {
    log("sched: appending node to list\n", WHITE);
    node->next = pos->next;
    node->prev = pos;
    if (pos->next) pos->next->prev = node;
    pos->next = node;
    if (list->tail == pos) list->tail = node;
    list->count++;
}

static void _node_delete(thread_list_t* list, list_node_t* node) {
    log("sched: deleting node from list\n", WHITE);
    if (list->head == node) list->head = node->next;
    if (list->tail == node) list->tail = node->prev;
    if (node->prev) node->prev->next = node->next;
    if (node->next) node->next->prev = node->prev;
    node->next = node->prev = NULL;
    list->count--;
}

static void _push_back(thread_list_t* list, list_node_t* node) {
    log("sched: pushing node to back of list\n", WHITE);
    if (!list->tail) {
        _initial_node(list, node);
    } else {
        _node_append(list, list->tail, node);
    }
}

static list_node_t* _pop_front(thread_list_t* list) {
    if (!list->head) return NULL;
    log("sched: popping node from front of list\n", WHITE);
    list_node_t* node = list->head;
    _node_delete(list, node);
    return node;
}

void thread_enqueue(thread_t* thread) {
    log("sched: enqueueing thread\n", WHITE);
    if (thread->state == THREAD_READY) {
        _push_back(&ready_list, &thread->node);
    }
}

void thread_dequeue(thread_t* thread) {
    log("sched: dequeueing thread\n", WHITE);
    list_node_t* current = ready_list.head;
    while (current) {
        if (current == &thread->node) {
            _node_delete(&ready_list, &thread->node);
            return;
        }
        current = current->next;
    }
}

thread_t* _create_initial_thread(void) {
    log("sched: creating initial thread\n", WHITE);
    thread_t* thread = thread_alloc();
    if (!thread) {
        log("sched: failed to allocate initial thread\n", WHITE);
        for (;;);
    }
    thread->id = 0;
    thread->state = THREAD_RUNNING;
    uint32_t esp;
    asm volatile("mov %%esp, %0" : "=r"(esp));
    thread->esp = esp;
    thread->base_esp = (uintptr_t)&stack_space - STACK_SIZE;
    return thread;
}

void scheduler_init(void) {
    log("sched: initializing scheduler\n", WHITE);
    thread_t* init = _create_initial_thread();
    current_thread = init;
    ready_list = THREAD_LIST_INIT;
    log("sched: scheduler initialized\n", WHITE);
}

void schedule(void) {
    log("sched: starting schedule\n", WHITE);
    
    // Clean up current thread if it's done
    if (current_thread && 
        (current_thread->state == THREAD_EXITED || current_thread->state == THREAD_DEAD)) {
        log("sched: current thread exited/dead, deallocating\n", WHITE);
        thread_t* old = current_thread;
        current_thread = NULL;
        dealloc_thread(old);
    }
    
    // Pick the next thread from ready list
    thread_t* next_thread = NULL;
    list_node_t* node = ready_list.head;
    while (node) {
        thread_t* t = LIST_CONTAINER_GET(node, thread_t, node);
        if (t->state == THREAD_READY) {
            next_thread = t;
            break;
        }
        node = node->next;
    }
    
    if (!next_thread) {
        if (current_thread && current_thread->state == THREAD_RUNNING) {
            log("sched: no ready thread, continuing current\n", WHITE);
            return;
        }
        log("sched: no ready thread and no running thread. System halted.\n", WHITE);
        for (;;);
    }

    log("sched: found next thread, preparing to switch\n", WHITE);

    if (current_thread && current_thread->state == THREAD_RUNNING) {
        log("sched: requeueing current thread\n", WHITE);
        current_thread->state = THREAD_READY;
        thread_enqueue(current_thread);
    }
    log("sched: removing next thread from ready list\n", WHITE);
    _node_delete(&ready_list, &next_thread->node);
    next_thread->state = THREAD_RUNNING;
    
    log("sched: calling context_switch\n", WHITE);
    
    context_switch(next_thread);
    

    log("sched: returned from context_switch\n", WHITE);
}

void yield(void) {
    log("sched: yield called\n", WHITE);
    schedule();
}
