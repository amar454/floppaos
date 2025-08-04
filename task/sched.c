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


static uint32_t cpu_core_count = 1;
#define MAX_CORES 8
static thread_list_t core_thread_lists[MAX_CORES];
static thread_t* core_current_thread[MAX_CORES];
static thread_t* core_idle_thread[MAX_CORES];
extern uint32_t global_tick_count;
static uint32_t next_thread_id = 1;

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

void sched_init() {
    cpu_core_count = get_cpu_core_count();
    if (cpu_core_count > MAX_CORES) cpu_core_count = MAX_CORES;
    for (uint32_t i = 0; i < cpu_core_count; ++i) {
        core_thread_lists[i].head = NULL;
        core_current_thread[i] = NULL;
    }
}

thread_t* sched_create_thread(void (*entry)(void)) {
    thread_t* t = (thread_t*)kmalloc(sizeof(thread_t));
    t->stack = (uint8_t*)kmalloc(THREAD_STACK_SIZE);
    t->ctx.eip = (uint32_t)entry;
    t->ctx.esp = (uint32_t)(t->stack + THREAD_STACK_SIZE);
    t->state = THREAD_READY;
    t->id = next_thread_id++;
    return t;
}

void sched_add_thread(thread_t* t) {
    uint32_t min_core = 0, min_count = (uint32_t)-1;
    for (uint32_t i = 0; i < cpu_core_count; ++i) {
        uint32_t count = 0;
        thread_t* iter = core_thread_lists[i].head;
        if (iter) {
            do {
                count++;
                iter = iter->next;
            } while (iter != core_thread_lists[i].head);
        }
        if (count < min_count) {
            min_count = count;
            min_core = i;
        }
    }
    thread_list_t* list = &core_thread_lists[min_core];
    if (!list->head) {
        list->head = t;
        t->next = t;
        t->prev = t;
    } else {
        thread_t* tail = list->head->prev;
        tail->next = t;
        t->prev = tail;
        t->next = list->head;
        list->head->prev = t;
    }
    t->core_id = min_core;
}

void sched_remove_thread(thread_t* t) {
    thread_list_t* list = &core_thread_lists[t->core_id];
    if (t->next == t) {
        list->head = NULL;
    } else {
        t->prev->next = t->next;
        t->next->prev = t->prev;
        if (list->head == t)
            list->head = t->next;
    }
}

uint32_t get_core_id() {
    uint32_t core_id;
    __asm__ volatile ("movl %%gs:0, %0" : "=r"(core_id));
    return core_id;
}

void sched_yield_core(uint32_t core_id) {
    thread_t* old = core_current_thread[core_id];
    thread_list_t* list = &core_thread_lists[core_id];

    if (!old) {
        old = core_idle_thread[core_id];
        core_current_thread[core_id] = old;
    }

    if (!list->head || !list->head->next) {
        thread_t* idle = core_idle_thread[core_id];
        if (idle && old != idle) {
            core_current_thread[core_id] = idle;
            ctx_switch(&old->ctx, &idle->ctx);
        }
        return;
    }

    thread_t* t = old->next ? old->next : list->head;
    thread_t* start = t;

    do {
        if (t->state == THREAD_READY)
            break;
        t = t->next ? t->next : list->head;
    } while (t != start);

    if (t->state != THREAD_READY)
        t = core_idle_thread[core_id];

    if (t && t != old) {
        core_current_thread[core_id] = t;
        ctx_switch(&old->ctx, &t->ctx);
    }
}



void sched_yield() {
    uint32_t core_id = get_core_id();
    if (core_id >= cpu_core_count) {
        log("sched_yield: Invalid core ID %u\n", core_id);
        return;
    }
    sched_yield_core(core_id);
}

thread_t* sched_get_current() {
    uint32_t core_id = get_core_id();
    return core_current_thread[core_id];
}

void sched_exit() {
    uint32_t core_id = get_core_id();
    thread_t* exiting = core_current_thread[core_id];
    if (!exiting) return;
    exiting->state = THREAD_EXITED;
    sched_remove_thread(exiting);
    sched_yield_core(core_id);
    kfree(exiting->stack, THREAD_STACK_SIZE);
    kfree(exiting, sizeof(thread_t));
}



void sched_start() {
    for (uint32_t i = 0; i < cpu_core_count; ++i) {
        thread_t* idle = sched_create_thread(idle_loop);
        idle->id = 0;
        idle->state = THREAD_READY;
        idle->core_id = i;
        core_idle_thread[i] = idle;
        sched_add_thread(idle);
        core_current_thread[i] = core_thread_lists[i].head;
        if (core_current_thread[i]->state == THREAD_READY)
            core_current_thread[i]->state = THREAD_RUNNING;
        void (*fn)(void) = (void (*)(void))core_current_thread[i]->ctx.eip;
        fn();
        sched_exit();
    }
}
