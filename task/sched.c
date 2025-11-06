#include "sched.h"
#include "thread.h"
#include "../mem/alloc.h"
#include "../mem/pmm.h"
#include "../mem/paging.h"
#include "../mem/vmm.h"
#include "../mem/utils.h"
#include "../lib/logging.h"
#include "../lib/str.h"
#include "../interrupts/interrupts.h"
#include "../task/sync/spinlock.h"
#include "../drivers/time/floptime.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

static void idle_thread_loop() {
    for (;;) {
    }
}

static void reaper_thread_entry() {}

static void stealer_thread_entry() {}

extern void context_switch(cpu_ctx_t* old, cpu_ctx_t* new);
extern void usermode_entry_routine(uint32_t stack, uint32_t ip);
scheduler_t sched;
#define USER_STACK_TOP 0xC0000000
#define USER_STACK_SIZE 0x1000

static uintptr_t sched_internal_alloc_user_stack(process_t* process, uint32_t stack_index) {
    uintptr_t user_stack_top = USER_STACK_TOP - (stack_index * USER_STACK_SIZE);
    uintptr_t phys = (uintptr_t) pmm_alloc_page();
    if (!phys) {
        return 0;
    }
    if (vmm_map(process->region, user_stack_top - USER_STACK_SIZE, phys, PAGE_RW | PAGE_USER) < 0) {
        pmm_free_page((void*) phys);
        return 0;
    }

    return user_stack_top;
}

static void sched_internal_setup_thread_stack(thread_t* thread, void (*entry)(void), uintptr_t user_stack_top) {
    uint32_t* kstack = (uint32_t*) ((uintptr_t) thread->kernel_stack + 4096);

    *--kstack = (uint32_t) entry;
    *--kstack = (uint32_t) user_stack_top;

    thread->context.eip = (uint32_t) usermode_entry_routine;
    thread->kernel_stack = kstack;
}

static inline uint32_t sched_internal_fetch_next_stack_index(process_t* process) {
    return process->threads ? process->threads->count : 0;
}

// set up thread lists
// init list spinlocks
// and create reaper and idle threads
void sched_init(void) {
    sched.ready_queue = kmalloc(sizeof(thread_list_t));
    sched.sleep_queue = kmalloc(sizeof(thread_list_t));
    sched.kernel_threads = kmalloc(sizeof(thread_list_t));
    sched.user_threads = kmalloc(sizeof(thread_list_t));
    flop_memset(sched.ready_queue, 0, sizeof(thread_list_t));
    flop_memset(sched.sleep_queue, 0, sizeof(thread_list_t));
    flop_memset(sched.kernel_threads, 0, sizeof(thread_list_t));
    flop_memset(sched.user_threads, 0, sizeof(thread_list_t));
    sched.ready_queue->name = "ready_queue";
    sched.sleep_queue->name = "sleep_queue";
    sched.kernel_threads->name = "kernel_threads";
    sched.user_threads->name = "user_threads";
    static spinlock_t ready_queue_spinlock_initializer = SPINLOCK_INIT;
    static spinlock_t sleep_queue_spinlock_initializer = SPINLOCK_INIT;
    static spinlock_t kernel_threads_spinlock_initializer = SPINLOCK_INIT;
    static spinlock_t user_threads_spinlock_initializer = SPINLOCK_INIT;
    spinlock_init(&sched.ready_queue->lock);
    spinlock_init(&sched.sleep_queue->lock);
    spinlock_init(&sched.kernel_threads->lock);
    spinlock_init(&sched.user_threads->lock);
    sched.idle_thread = sched_create_kernel_thread(&idle_thread_loop, 5, "idle");
    sched.reaper_thread = sched_create_kernel_thread(&reaper_thread_entry, 2, "reaper");
    sched.stealer_thread = NULL;
    sched.next_tid = 0;
    sched_thread_list_add(sched.reaper_thread, sched.ready_queue);
    sched_thread_list_add(sched.idle_thread, sched.ready_queue);

    log("sched init - ok", GREEN);
}

void sched_enqueue(thread_list_t* list, thread_t* thread) {
    if (!list || !thread) {
        return;
    }

    // we must lock when accessesing a thread list
    // this can lead to a race condition if many cores access it at once
    // this isnt a huge concern for now but it does disable interrupts
    // which is important for us here.
    spinlock(&list->lock);

    thread->next = NULL;

    if (!list->head) {
        list->head = thread;
        list->tail = thread;
    } else {
        list->tail->next = thread;
        list->tail = thread;
    }

    // atomic addition for increasing list->count
    atomic_fetch_add_explicit((atomic_uint*) &list->count, 1, memory_order_release);

    spinlock_unlock(&list->lock, true);
}

// remove head of a thread queue
thread_t* sched_dequeue(thread_list_t* list) {
    if (!list) {
        return NULL;
    }

    // we must lock when accessesing a thread list
    spinlock(&list->lock);

    thread_t* thread = list->head;
    if (!thread) {
        spinlock_unlock(&list->lock, true);
        return NULL;
    }

    list->head = thread->next;

    // if the list is empty we set list->tail to NULL which
    // signifies the list is empty.
    if (!list->head) {
        list->tail = NULL;
    }

    atomic_fetch_sub_explicit((atomic_uint*) &list->count, 1, memory_order_release);

    thread->next = NULL;

    spinlock_unlock(&list->lock, true);

    return thread;
}

// remove arbritrary thread_t* "target" from queue.
// slower than dequeue because its (o(n))
// its probably better to use dequeue()
thread_t* sched_remove(thread_list_t* list, thread_t* target) {
    if (!list || !target) {
        return NULL;
    }
    spinlock(&list->lock);
    thread_t* prev = NULL;
    thread_t* curr = list->head;
    while (curr) {
        if (curr == target) {
            if (prev) {
                prev->next = curr->next;
            } else {
                list->head = curr->next;
            }

            if (curr == list->tail) {
                list->tail = prev;
            }

            list->count--;
            curr->next = NULL;

            spinlock_unlock(&list->lock, true);
            return curr;
        }
        prev = curr;
        curr = curr->next;
    }
    spinlock_unlock(&list->lock, true);
    return NULL;
}

static thread_t*
sched_internal_init_thread(void (*entry)(void), unsigned priority, char* name, int user, process_t* process) {
    // todo: create slab cache for thread structs
    thread_t* this_thread = kmalloc(sizeof(thread_t));
    this_thread->next = NULL;
    this_thread->previous = NULL;
    this_thread->kernel_stack = (void*) kmalloc(4096);
    if (!this_thread->kernel_stack) {
        // stack allocation failed
        return NULL;
    }

    // effective priority starts the same as base priority
    // it will be boosted if the thread is starved
    // the scheduler will always use the effective priority
    // the base priority is simply the priority assigned upon thread creation
    this_thread->priority.base = priority;
    this_thread->priority.effective = priority;
    cpu_ctx_t this_thread_context = {.edi = 0,
                                     .esi = 0,
                                     .ebx = 0,
                                     .ebp = 0,
                                     // user threads will set eip to the user mode entry routine, use wrapper.
                                     .eip = (uint32_t) entry};

    this_thread->context = this_thread_context;

    if (user) {
        this_thread->user = 1;
        if (process == NULL) {
            // user threads require a process
            return NULL;
        }
        this_thread->process = process;
    } else {
        this_thread->user = 0;
        if (process != NULL) {
            return NULL;
            // kernel threads cannot have a process
            // that's actually the only thing that makes them kernel threads.
        }
        this_thread->process = NULL;
        // this must remain null.
    }

    this_thread->id = sched.next_tid;
    sched.next_tid++;

    this_thread->name = name;
    this_thread->uptime = 0;
    this_thread->time_since_last_run = 0;
    this_thread->time_slice = priority * 2;

    return this_thread;
}

thread_t* sched_create_user_thread(void (*entry)(void), unsigned priority, char* name, process_t* process) {
    if (!process)
        return NULL;

    thread_t* new_thread = sched_internal_init_thread((void*) usermode_entry_routine, priority, name, 1, process);

    if (!new_thread) {
        return NULL;
    }

    uint32_t stack_index = sched_internal_fetch_next_stack_index(process);
    uintptr_t user_stack_top = sched_internal_alloc_user_stack(process, stack_index);

    if (!user_stack_top) {
        kfree(new_thread, sizeof(thread_t));
        return NULL;
    }

    sched_internal_setup_thread_stack(new_thread, entry, user_stack_top);

    // in addition to adding this thread to the thread queue in enqueue()
    // we need to add it to the linked list of threads for the process.
    // NOTE: this is not a thread queue. it is a list of threads for each process.
    sched_thread_list_add(new_thread, process->threads);

    // we must also add it to the linked list of user threads
    sched_thread_list_add(new_thread, sched.user_threads);
    log("sched: user thread created", GREEN);
    return new_thread;
}

void sched_thread_list_add(thread_t* thread, thread_list_t* list) {
    if (!thread || !list)
        return;

    spinlock(&list->lock);

    thread->next = NULL;

    if (!list->head) {
        list->head = thread;
        list->tail = thread;
    } else {
        list->tail->next = thread;
        list->tail = thread;
    }

    list->count++;

    spinlock_unlock(&list->lock, true);
}

thread_t* sched_create_kernel_thread(void (*entry)(void), unsigned priority, char* name) {
    thread_t* new_thread = sched_internal_init_thread((void*) entry, priority, name, 0, NULL);
    log("sched: kernel thread created", GREEN);
    sched_thread_list_add(new_thread, sched.kernel_threads);
    return new_thread;
}

thread_t* current_thread;

// our context switch takes our cpu context struct which
// contains edi, esi, ebx, ebp, eip
// we do not need to save esp because we have it in thread->kernel_stack
extern void context_switch(cpu_ctx_t* old, cpu_ctx_t* new);
extern void usermode_entry_routine(uint32_t sp, uint32_t ip);

void sched_tick(void) {
    if (--current_thread->time_slice == 0) {
        current_thread->time_slice = 10;
        sched_yield();
    }
}

void sched_boost_starved_threads(thread_list_t* list) {
    spinlock(&list->lock);

    for (thread_t* t = list->head; t; t = t->next) {
        t->time_since_last_run++;

        // if the threads time since last run
        // is greater than the starvation threadhold
        // we will boost its effective priority
        if (t->time_since_last_run > STARVATION_THRESHOLD && t->priority.effective < MAX_PRIORITY) {
            t->priority.effective += BOOST_AMOUNT;
        }
    }

    spinlock_unlock(&list->lock, true);
}

static thread_t* sched_select_highest_priority(thread_list_t* list) {
    thread_t* iter = list->head;
    thread_t* best = iter;
    thread_t* best_prev = NULL;
    thread_t* prev = NULL;

    while (iter) {
        if (iter->priority.effective > best->priority.effective) {
            best_prev = prev;
            best = iter;
        }
        prev = iter;
        iter = iter->next;
    }

    if (!best) {
        return NULL;
    }

    // detach best
    if (best_prev) {
        best_prev->next = best->next;
    } else {
        list->head = best->next;
    }

    if (best == list->tail) {
        list->tail = best_prev;
    }

    list->count--;
    best->next = NULL;
    return best;
}

void sched_schedule(void) {
    sched_boost_starved_threads(sched.ready_queue);

    spinlock(&sched.ready_queue->lock);
    thread_t* next = sched_select_highest_priority(sched.ready_queue);
    spinlock_unlock(&sched.ready_queue->lock, true);

    if (!next) {
        next = sched.idle_thread;
    }
    if (next == current_thread) {
        return;
    }

    next->priority.effective = next->priority.base;
    next->time_since_last_run = 0;
    thread_t* prev = current_thread;
    current_thread = next;

    context_switch(&prev->context, &next->context);
}

void sched_yield(void) {
    if (!current_thread) {
        return;
    }
    if (current_thread != sched.idle_thread) {
        sched_enqueue(sched.ready_queue, current_thread);
    }

    sched_schedule();
}
