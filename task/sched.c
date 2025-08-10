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

scheduler_t sched;
static uint64_t global_ticks = 0;

static void idle_loop() {
    while (1) {
        asm volatile ("sti; hlt");
    }
}

static uint32_t get_cpu_core_count() {
    uint32_t eax, ebx, ecx, edx;
    eax = 1;
    __asm__ volatile (
        "cpuid"
        : "+a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        :
        :
    );
    uint32_t logical = (ebx >> 16) & 0xff;
    return logical > 0 ? logical : 1;
}



void ctx_switch(cpu_ctx_t* old, cpu_ctx_t* new_ctx) {
    asm volatile (
        "pushf\n"
        "pusha\n"
        "movl %%esp, %0\n"
        "movl %1, %%esp\n"
        "popa\n"
        "popf\n"
        "iret\n"
        : "=m"(old->esp)
        : "m"(new_ctx->esp)
        : "memory"
    );
}
static spinlock_t _sched_lock_init = SPINLOCK_INIT;
void sched_init() {
    sched.core_count = get_cpu_core_count();
    if (sched.core_count > MAX_CORES) 
        sched.core_count = MAX_CORES;
    
    sched.thread_count = 0;
    sched.next_tid = 1;
    sched.sched_lock = _sched_lock_init;

    spinlock_init(&sched.sched_lock);
    
    for (uint32_t i = 0; i < sched.core_count; ++i) {
        sched.ready_queues[i].head = NULL;
        sched.ready_queues[i].tail = NULL;
        sched.ready_queues[i].count = 0;
        sched.current_threads[i] = NULL;
        sched.idle_threads[i] = NULL;
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
    t->priority = priority;
    t->time_slice = 10 + priority;
    t->cpu_usage = 0;
    t->last_run = 0;
    t->wake_time = 0;
    
    return t;
}

void sched_add_thread(thread_t* t) {
    spinlock(&sched.sched_lock);
    
    uint32_t target_core = 0;
    uint32_t min_load = (uint32_t)-1;
    
    for (uint32_t i = 0; i < sched.core_count; ++i) {
        if (sched.ready_queues[i].count < min_load) {
            min_load = sched.ready_queues[i].count;
            target_core = i;
        }
    }
    
    t->core_id = target_core;
    
    if (!sched.ready_queues[target_core].head) {
        sched.ready_queues[target_core].head = t;
        sched.ready_queues[target_core].tail = t;
        t->next = t;
        t->prev = t;
    } else {
        thread_t* tail = sched.ready_queues[target_core].tail;
        tail->next = t;
        t->prev = tail;
        t->next = sched.ready_queues[target_core].head;
        sched.ready_queues[target_core].head->prev = t;
        sched.ready_queues[target_core].tail = t;
    }
    
    sched.ready_queues[target_core].count++;
    sched.thread_count++;
    
    spinlock_unlock(&sched.sched_lock, true);
}

void sched_remove_thread(thread_t* t) {
    spinlock(&sched.sched_lock);
    
    uint32_t core = t->core_id;
    thread_list_t* list = &sched.ready_queues[core];
    
    if (t->next == t) {
        list->head = NULL;
        list->tail = NULL;
    } else {
        t->prev->next = t->next;
        t->next->prev = t->prev;
        if (list->head == t) list->head = t->next;
        if (list->tail == t) list->tail = t->prev;
    }
    
    list->count--;
    sched.thread_count--;
    
    spinlock_unlock(&sched.sched_lock, true);
}

uint32_t sched_get_core_id() {
    uint32_t core_id;
    asm volatile ("movl %%fs, %0" : "=r"(core_id));
    return core_id;
}

static inline scheduler_t* get_core_sched() {
    uint32_t core_id = sched_get_core_id();
    if (core_id >= sched.core_count) {
        return NULL;
    }
    return &sched;
}

static void sched_reschedule_core(uint32_t core_id) {
    if (core_id >= sched.core_count) return;
    
    thread_t* old = sched.current_threads[core_id];
    thread_list_t* ready = &sched.ready_queues[core_id];
    
    if (!old) {
        old = sched.idle_threads[core_id];
        sched.current_threads[core_id] = old;
    }
    
    if (!ready->head) {
        if (old != sched.idle_threads[core_id]) {
            sched.current_threads[core_id] = sched.idle_threads[core_id];
            ctx_switch(&old->ctx, &sched.idle_threads[core_id]->ctx);
        }
        return;
    }
    
    thread_t* next = ready->head;
    thread_t* start = next;
    
    do {
        if (next->state == THREAD_READY) break;
        next = next->next;
    } while (next != start);
    
    if (next->state != THREAD_READY) {
        next = sched.idle_threads[core_id];
    }
    
    if (next && next != old) {
        sched.current_threads[core_id] = next;
        old->state = THREAD_READY;
        next->state = THREAD_RUNNING;
        next->last_run = global_ticks;
        ctx_switch(&old->ctx, &next->ctx);
    }
}

void schedule() {
    uint32_t core_id = sched_get_core_id();
    sched_reschedule_core(core_id);
}

void sched_yield() {
    schedule();
}

void sched_exit() {
    uint32_t core_id = sched_get_core_id();
    thread_t* exiting = sched.current_threads[core_id];
    
    if (!exiting) return;
    
    exiting->state = THREAD_EXITED;
    sched_remove_thread(exiting);
    
    kfree(exiting->stack, THREAD_STACK_SIZE);
    kfree(exiting, sizeof(thread_t));
    
    schedule();
}

void sched_sleep(uint64_t ms) {
    uint32_t core_id = sched_get_core_id();
    thread_t* current = sched.current_threads[core_id];
    
    if (!current) return;
    
    current->wake_time = global_ticks + ms;
    current->state = THREAD_SLEEPING;
    
    spinlock(&sched.sched_lock);
    
    if (!sched.sleep_queue.head) {
        sched.sleep_queue.head = current;
        sched.sleep_queue.tail = current;
        current->next = current;
        current->prev = current;
    } else {
        thread_t* tail = sched.sleep_queue.tail;
        tail->next = current;
        current->prev = tail;
        current->next = sched.sleep_queue.head;
        sched.sleep_queue.head->prev = current;
        sched.sleep_queue.tail = current;
    }
    
    sched.sleep_queue.count++;
    
    spinlock_unlock(&sched.sched_lock, true);
    
    schedule();
}

void sched_wake_expired() {
    spinlock(&sched.sched_lock);
    
    thread_t* current = sched.sleep_queue.head;
    if (!current) {
        spinlock_unlock(&sched.sched_lock, true);
        return;
    }
    
    thread_t* start = current;
    uint64_t now = global_ticks;
    
    do {
        thread_t* next = current->next;
        
        if (current->wake_time <= now) {
            if (current->prev == current) {
                sched.sleep_queue.head = NULL;
                sched.sleep_queue.tail = NULL;
            } else {
                current->prev->next = current->next;
                current->next->prev = current->prev;
                if (sched.sleep_queue.head == current) 
                    sched.sleep_queue.head = current->next;
                if (sched.sleep_queue.tail == current) 
                    sched.sleep_queue.tail = current->prev;
            }
            
            sched.sleep_queue.count--;
            current->state = THREAD_READY;
            sched_add_thread(current);
        }
        
        current = next;
    } while (current != start);
    
    spinlock_unlock(&sched.sched_lock, true);
}

void sched_timer_tick() {
    global_ticks++;
    sched_wake_expired();
    
    for (uint32_t i = 0; i < sched.core_count; ++i) {
        if (sched.ready_queues[i].count > 0) {
            sched_reschedule(i);
        }
    }
}

void sched_balance_load() {
    spinlock(&sched.sched_lock);
    for (uint32_t i = 0; i < sched.core_count; ++i) {
        if (sched.ready_queues[i].count > 1) {
            thread_list_t* list = &sched.ready_queues[i];
            thread_t* current = list->head;
            thread_t* next = current->next;
            while (current != list->tail) {
                if (current->state == THREAD_READY && next->state == THREAD_READY) {
                    // Move next to a less loaded core      
                    uint32_t target_core = (i + 1) % sched.core_count;
                    while (sched.ready_queues[target_core].count >= sched.ready_queues[i].count) {
                        target_core = (target_core + 1) % sched.core_count;
                    }
                    sched_remove_thread(next);
                    next->core_id = target_core;
                    sched_add_thread(next);
                }
                current = next;
                next = next->next;
            }
        }
    }       
    spinlock_unlock(&sched.sched_lock, true);
}

void sched_start() {
    for (uint32_t i = 0; i < sched.core_count; ++i) {
        sched.idle_threads[i] = sched_create_thread(idle_loop, 0);
        sched.idle_threads[i]->state = THREAD_RUNNING;
        sched.current_threads[i] = sched.idle_threads[i];
    }
    sched_balance_load();
}