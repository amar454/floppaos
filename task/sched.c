#include "sched.h"
#include "thread.h"
#include "../mem/alloc.h"
#include "../mem/pmm.h"
#include "../mem/vmm.h"
#include "../lib/logging.h"
#include "../lib/str.h"
#include "../interrupts/interrupts.h"
#include <stdint.h>
#include <stddef.h>

thread_t* thread_list_head = NULL;
thread_t* current_thread = NULL;
thread_t* idle_thread = NULL;
uint32_t next_thread_id = 1;

extern uint32_t global_tick_count;

static void idle_loop() {
    while (1) {
        asm volatile ("hlt");
    }
}
void ctx_switch(cpu_ctx_t* _old, cpu_ctx_t* _new) {
    asm volatile (
        "pushf\n"
        "pusha\n"
        "movl %%esp, %[old]\n"
        "movl %[new], %%esp\n"
        "popa\n"
        "popf\n"
        "ret\n"
        : [old] "=m"(*_old)
        : [new] "m"(*_new)
        : "memory"
    );
}
void sched_init() {
    thread_list_head = NULL;
    current_thread = NULL;
    next_thread_id = 1;
    idle_thread = sched_create_thread(idle_loop);
    idle_thread->id = 0;
    idle_thread->state = THREAD_READY;
}

thread_t* sched_create_thread(void (*entry)(void)) {
    thread_t* t = (thread_t*)kmalloc(sizeof(thread_t));
    t->stack = (uint8_t*)kmalloc(THREAD_STACK_SIZE);
    t->ctx.eip = (uint32_t)entry;
    t->ctx.esp = (uint32_t)(t->stack + THREAD_STACK_SIZE);
    t->state = THREAD_READY;
    t->wake_tick = 0;
    t->id = next_thread_id++;
    sched_add_thread(t);
    return t;
}

void sched_add_thread(thread_t* t) {
    if (!thread_list_head) {
        thread_list_head = t;
        t->next = t;
        t->prev = t;
    } else {
        thread_t* tail = thread_list_head->prev;
        tail->next = t;
        t->prev = tail;
        t->next = thread_list_head;
        thread_list_head->prev = t;
    }
}

void sched_remove_thread(thread_t* t) {
    if (t->next == t) {
        thread_list_head = NULL;
    } else {
        t->prev->next = t->next;
        t->next->prev = t->prev;
        if (thread_list_head == t)
            thread_list_head = t->next;
    }
}

void sched_yield() {
    if (!current_thread) return;
    thread_t* old = current_thread;
    thread_t* t = current_thread->next;
    while (t != current_thread) {
        if (t->state == THREAD_READY)
            break;
        if (t->state == THREAD_SLEEPING && global_tick_count >= t->wake_tick) {
            t->state = THREAD_READY;
            break;
        }
        t = t->next;
    }
    if (t == current_thread && current_thread->state != THREAD_READY)
        t = idle_thread;
    if (t != current_thread) {
        current_thread = t;
        ctx_switch(&old->ctx, &current_thread->ctx);
    }
}

thread_t* sched_get_current() {
    return current_thread;
}

void sched_sleep(uint32_t ticks) {
    if (!current_thread) return;
    current_thread->state = THREAD_SLEEPING;
    current_thread->wake_tick = global_tick_count + ticks;
    sched_yield();
}

void sched_exit() {
    if (!current_thread) return;
    thread_t* exiting = current_thread;
    current_thread->state = THREAD_EXITED;
    sched_remove_thread(current_thread);
    sched_yield();
    kfree(exiting->stack, THREAD_STACK_SIZE);
    kfree(exiting, sizeof(thread_t));
}

void sched_tick() {
    thread_t* t = thread_list_head;
    if (!t) return;
    do {
        if (t->state == THREAD_SLEEPING && global_tick_count >= t->wake_tick)
            t->state = THREAD_READY;
        t = t->next;
    } while (t != thread_list_head);
}

void sched_start() {
    if (!thread_list_head) return;
    current_thread = thread_list_head;
    if (current_thread->state == THREAD_READY)
        current_thread->state = THREAD_RUNNING;
    void (*fn)(void) = (void (*)(void))current_thread->ctx.eip;
    fn();
    sched_exit();
}