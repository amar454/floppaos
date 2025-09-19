#include "sched.h"
#include "thread.h"
#include "../mem/alloc.h"
#include "../mem/pmm.h"
#include "../mem/vmm.h"
#include "../lib/logging.h"
#include "../lib/str.h"
#include "../interrupts/interrupts.h"
#include "../task/sync/spinlock.h"
#include "../drivers/time/floptime.h"
#include <stdint.h>
#include <stddef.h>

#define STARVATION_THRESHOLD 50 
#define BASE_QUANTUM 10
#define GOLD_QUANTUM 20

scheduler_t sched;
static uint64_t global_ticks = 0;

 thread_list_t ready_queue[MAX_CORES];
 thread_t* current_thread[MAX_CORES];
 thread_t* idle_thread[MAX_CORES];

static void idle_loop() {
    while (1) {
        asm volatile("sti; hlt");
    }
}

void ctx_switch(cpu_ctx_t* old, cpu_ctx_t* new_ctx) {
    asm volatile (
        "pushf\n"
        "pusha\n"
        "movl %%esp, %0\n"
        "movl %1, %%esp\n"
        "popa\n"
        "popf\n"
        "ret\n"
        : "=m"(old->esp)
        : "m"(new_ctx->esp)
        : "memory"
    );
}

static inline void set_thread_quantum(thread_t* t) {
    uint64_t waited = global_ticks - t->ready_since;
    if (waited > STARVATION_THRESHOLD) {
        t->time_slice = GOLD_QUANTUM;
    } else {
        t->time_slice = BASE_QUANTUM;
    }
}

static thread_t* pick_next_thread(uint32_t core) {
    thread_t* best = idle_thread[core];
    thread_t* cur = ready_queue[core].head;
    if (!cur) return best;

    thread_t* start = cur;
    do {
        if (cur->state == THREAD_READY) {
            uint64_t waited = global_ticks - cur->ready_since;
            cur->effective_prio = cur->base_priority;
            if (waited > STARVATION_THRESHOLD) cur->effective_prio += 1;

            if (!best || cur->effective_prio > best->effective_prio ||
                (cur->effective_prio == best->effective_prio &&
                 waited > (global_ticks - best->ready_since))) {
                best = cur;
            }
        }
        cur = cur->next;
    } while (cur != start);

    return best;
}


static spinlock_t initializer_spinlock = SPINLOCK_INIT;

void sched_init(uint32_t core_count) {
    sched.core_count = core_count;
    sched.thread_count = 0;
    sched.next_tid = 1;
    sched.sched_lock = initializer_spinlock;
    spinlock_init(&sched.sched_lock);

    for (uint32_t i = 0; i < core_count; i++) {
        ready_queue[i].head = NULL;
        ready_queue[i].tail = NULL;
        ready_queue[i].count = 0;

        current_thread[i] = NULL;
        idle_thread[i] = NULL;
    }

    sched.sleep_queue.head = NULL;
    sched.sleep_queue.tail = NULL;
    sched.sleep_queue.count = 0;
}

thread_t* sched_create_thread(void (*entry)(void), uint32_t priority) {
    thread_t* t = (thread_t*)kmalloc(sizeof(thread_t));
    t->stack = (uint8_t*)kmalloc(THREAD_STACK_SIZE);

    t->ctx.eip = (uint32_t)entry;
    t->ctx.esp = (uint32_t)(t->stack + THREAD_STACK_SIZE - 16);
    t->ctx.eflags = 0x202;

    t->state = THREAD_READY;
    t->id = sched.next_tid++;
    t->base_priority = priority;
    t->effective_prio = priority;
    t->time_slice = BASE_QUANTUM;
    t->ready_since = global_ticks;
    t->run_count = 0;
    t->last_run = 0;
    t->wake_time = 0;
    t->core_id = 0;

    t->next = t;
    t->prev = t;

    return t;
}

void sched_add_thread(thread_t* t) {
    spinlock(&sched.sched_lock);
    uint32_t core = t->core_id;

    if (!ready_queue[core].head) {
        ready_queue[core].head = t;
        ready_queue[core].tail = t;
        t->next = t;
        t->prev = t;
    } else {
        thread_t* tail = ready_queue[core].tail;
        tail->next = t;
        t->prev = tail;
        t->next = ready_queue[core].head;
        ready_queue[core].head->prev = t;
        ready_queue[core].tail = t;
    }

    ready_queue[core].count++;
    sched.thread_count++;
    t->ready_since = global_ticks;

    spinlock_unlock(&sched.sched_lock, true);
}

void sched_remove_thread(thread_t* t) {
    spinlock(&sched.sched_lock);
    uint32_t core = t->core_id;

    if (t->next == t) {
        ready_queue[core].head = NULL;
        ready_queue[core].tail = NULL;
    } else {
        t->prev->next = t->next;
        t->next->prev = t->prev;
        if (ready_queue[core].head == t) { 
            ready_queue[core].head = t->next; 
        }
        if (ready_queue[core].tail == t) {
            ready_queue[core].tail = t->prev;
        }
    }
    ready_queue[core].count--;
    sched.thread_count--;

    spinlock_unlock(&sched.sched_lock, true);
}

void sched_reschedule(uint32_t core) {
    thread_t* old = current_thread[core];
    thread_t* next = pick_next_thread(core);
    if (!next || next == old) return;

    if (next != idle_thread[core]) {
        sched_remove_thread(next);
    }

    if (old && old->state == THREAD_RUNNING) {
        old->state = THREAD_READY;
        old->ready_since = global_ticks;
        sched_add_thread(old);
    }

    set_thread_quantum(next);
    next->state = THREAD_RUNNING;
    next->last_run = global_ticks;
    next->run_count++;
    current_thread[core] = next;

    ctx_switch(&old->ctx, &next->ctx);
}

void sched_yield(uint32_t core) {
    sched_reschedule(core);
}

void sched_exit(uint32_t core) {
    thread_t* t = current_thread[core];
    if (!t) return;

    t->state = THREAD_DEAD;
    sched_remove_thread(t);

    kfree(t->stack, THREAD_STACK_SIZE);
    kfree(t, sizeof(thread_t));

    sched_reschedule(core);
}

void sched_sleep(uint64_t ms) {
    thread_t* t = current_thread[0];
    if (!t) return;

    t->wake_time = global_ticks + ms;
    t->state = THREAD_SLEEPING;

    spinlock(&sched.sched_lock);
    if (!sched.sleep_queue.head) {
        sched.sleep_queue.head = t;
        sched.sleep_queue.tail = t;
        t->next = t;
        t->prev = t;
    } else {
        thread_t* tail = sched.sleep_queue.tail;
        tail->next = t;
        t->prev = tail;
        t->next = sched.sleep_queue.head;
        sched.sleep_queue.head->prev = t;
        sched.sleep_queue.tail = t;
    }
    sched.sleep_queue.count++;
    spinlock_unlock(&sched.sched_lock, true);

    sched_reschedule(t->core_id);
}

void sched_wake(thread_t* t) {
    spinlock(&sched.sched_lock);
    if (t->state != THREAD_SLEEPING) {
        spinlock_unlock(&sched.sched_lock, true);
        return;
    }

    t->state = THREAD_READY;
    t->effective_prio = t->base_priority;
    sched_add_thread(t);
    spinlock_unlock(&sched.sched_lock, true);
}

void sched_timer_tick() {
    global_ticks++;
}

void sched_start(uint32_t core) {
    idle_thread[core] = sched_create_thread(idle_loop, 0);
    idle_thread[core]->state = THREAD_RUNNING;
    current_thread[core] = idle_thread[core];
}
