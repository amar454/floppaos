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

int sched_spinlocks_init(void) {
    static spinlock_t ready_queue_spinlock_initializer = SPINLOCK_INIT;
    static spinlock_t sleep_queue_spinlock_initializer = SPINLOCK_INIT;
    static spinlock_t kernel_threads_spinlock_initializer = SPINLOCK_INIT;
    static spinlock_t user_threads_spinlock_initializer = SPINLOCK_INIT;
    spinlock_init(&sched.ready_queue->lock);
    spinlock_init(&sched.sleep_queue->lock);
    spinlock_init(&sched.kernel_threads->lock);
    spinlock_init(&sched.user_threads->lock);
}

int sched_scheduler_lists_init(void) {
    sched.ready_queue = kmalloc(sizeof(thread_list_t));
    if (!sched.ready_queue)
        return -1;
    sched.sleep_queue = kmalloc(sizeof(thread_list_t));
    if (!sched.sleep_queue)
        return -1;
    sched.kernel_threads = kmalloc(sizeof(thread_list_t));
    if (!sched.kernel_threads)
        return -1;
    sched.user_threads = kmalloc(sizeof(thread_list_t));
    if (!sched.user_threads)
        return -1;

    if (sched_spinlocks_init() < 0) {
        return -1;
    }
    flop_memset(sched.ready_queue, 0, sizeof(thread_list_t));
    flop_memset(sched.sleep_queue, 0, sizeof(thread_list_t));
    flop_memset(sched.kernel_threads, 0, sizeof(thread_list_t));
    flop_memset(sched.user_threads, 0, sizeof(thread_list_t));
    return 0;
}

int sched_scheduler_lists_name_init(void) {
    sched.ready_queue->name = "ready_queue";
    sched.sleep_queue->name = "sleep_queue";
    sched.kernel_threads->name = "kernel_threads";
    sched.user_threads->name = "user_threads";
    return 0;
}

int sched_init_kernel_worker_pool(void);

// set up thread lists
// init list spinlocks
// and create reaper and idle threads
void sched_init(void) {
    if (sched_scheduler_lists_init() < 0) {
        log("sched: failed to init scheduler lists\n", RED);
        return;
    }

    if (sched_scheduler_lists_name_init() < 0) {
        log("sched: failed to init scheduler list names\n", RED);
        return;
    }

    if (sched_init_worker_threads() < 0) {
        log("sched: failed to init worker threads\n", RED);
        return;
    }

    if (sched_init_kernel_worker_pool() < 0) {
        log("sched: failed to init kernel worker pool\n", RED);
        return;
    }

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

typedef enum worker_pref {
    SUBSYSTEM_MEM,
    SUBSYSTEM_IO,
    SUBSYSTEM_PROC,
    SUBSYSTEM_FS,
    SUBSYSTEM_NONE
} worker_pref_t;

typedef struct sched_worker_thread {
    thread_t* thread;
    void (*entry)(void*);
    void* arg;
    worker_pref_t subsystem_pref;
} worker_thread;

static worker_thread* sched_internal_init_worker(
    void (*entry)(void*), void* arg, unsigned priority, char* name, worker_pref_t subsystem_pref) {
    worker_thread* worker_thread = kmalloc(sizeof(worker_thread));
    if (!worker_thread) {
        return NULL;
    }

    thread_t* thread = sched_internal_init_thread((void*) entry, priority, name, 0, NULL);
    if (!thread) {
        kfree(worker_thread, sizeof(worker_thread));
        return NULL;
    }

    worker_thread->thread = thread;
    worker_thread->entry = entry;
    worker_thread->arg = arg;
    worker_thread->subsystem_pref = subsystem_pref;

    return worker_thread;
}

static worker_thread* sched_create_worker_thread(
    void (*entry)(void*), void* arg, unsigned priority, char* name, worker_pref_t subsystem_pref) {
    worker_thread* worker_thread = sched_internal_init_worker(entry, arg, priority, name, subsystem_pref);
    if (!worker_thread) {
        return NULL;
    }

    sched_thread_list_add(worker_thread->thread, sched.kernel_threads);
    return worker_thread;
}

worker_thread** sched_create_worker_pool(
    size_t count, void (*entry)(void*), void** args, unsigned priority, char* name, worker_pref_t subsystem_pref) {
    if (count == 0 || !entry)
        return NULL;

    worker_thread** pool = kmalloc(sizeof(worker_thread*) * count);
    if (!pool)
        return NULL;

    size_t created = 0;
    for (size_t i = 0; i < count; ++i) {
        void* arg = args ? args[i] : NULL;
        worker_thread* w = sched_create_worker_thread(entry, arg, priority, name, subsystem_pref);
        if (!w) {
            for (size_t j = 0; j < created; ++j) {
                worker_thread* cw = pool[j];
                if (!cw)
                    continue;

                if (cw->thread) {
                    sched_remove(sched.kernel_threads, cw->thread);

                    if (cw->thread->kernel_stack) {
                        kfree(cw->thread->kernel_stack, 4096);
                    }
                    kfree(cw->thread, sizeof(thread_t));
                }

                kfree(cw, sizeof(worker_thread));
            }
            kfree(pool, sizeof(worker_thread*) * count);
            return NULL;
        }

        sched_enqueue(sched.ready_queue, w->thread);

        pool[created++] = w;
    }

    return pool;
}

// Expand worker pool to target_count. Returns 0 on success, -1 on failure.
int sched_expand_worker_pool(worker_thread*** pool_ptr,
                             size_t* count_ptr,
                             size_t target_count,
                             void (*entry)(void*),
                             void** args,
                             unsigned priority,
                             char* name,
                             worker_pref_t subsystem_pref) {
    if (!pool_ptr || !count_ptr || target_count == 0)
        return -1;

    size_t old_count = *count_ptr;
    if (old_count >= target_count)
        return 0;

    worker_thread** old_pool = *pool_ptr;
    worker_thread** new_pool = kmalloc(sizeof(worker_thread*) * target_count);
    if (!new_pool)
        return -1;

    for (size_t i = 0; i < old_count; ++i) {
        new_pool[i] = old_pool ? old_pool[i] : NULL;
    }

    size_t created = 0;
    for (size_t i = old_count; i < target_count; ++i) {
        void* arg = args ? args[i] : NULL;
        worker_thread* w = sched_create_worker_thread(entry, arg, priority, name, subsystem_pref);
        if (!w) {
            for (size_t j = old_count; j < old_count + created; ++j) {
                worker_thread* cw = new_pool[j];
                if (!cw)
                    continue;

                if (cw->thread) {
                    sched_remove(sched.kernel_threads, cw->thread);
                    if (cw->thread->kernel_stack) {
                        kfree(cw->thread->kernel_stack, 4096);
                    }
                    kfree(cw->thread, sizeof(thread_t));
                }
                kfree(cw, sizeof(worker_thread));
            }
            kfree(new_pool, sizeof(worker_thread*) * target_count);
            return -1;
        }

        sched_enqueue(sched.ready_queue, w->thread);
        new_pool[i] = w;
        created++;
    }

    // replace pool pointer
    if (old_pool) {
        kfree(old_pool, sizeof(worker_thread*) * old_count);
    }
    *pool_ptr = new_pool;
    *count_ptr = target_count;
    return 0;
}

typedef struct worker_pool_descriptor {
    worker_thread*** pool_ptr;
    size_t* count_ptr;
    size_t min_count;
    size_t grow_by;
    void (*entry)(void*);
    void** args;
    unsigned priority;
    char* name;
    worker_pref_t pref;
} worker_pool_descriptor_t;

static void worker_pool_manager_entry(void* _arg) {
    worker_pool_descriptor_t* a = (worker_pool_descriptor_t*) _arg;
    if (!a) {
        return;
    }

    for (;;) {
        size_t cur = a->count_ptr ? *a->count_ptr : 0;
        if (cur < a->min_count) {
            size_t target = cur + (a->grow_by ? a->grow_by : a->min_count - cur);
            if (target < a->min_count)
                target = a->min_count;
            sched_expand_worker_pool(
                a->pool_ptr, a->count_ptr, target, a->entry, a->args, a->priority, a->name, a->pref);
        }

        sched_yield();
    }
}

int sched_start_worker_pool_manager(worker_thread*** pool_ptr,
                                    size_t* count_ptr,
                                    size_t min_count,
                                    size_t grow_by,
                                    void (*entry)(void*),
                                    void** args,
                                    unsigned priority,
                                    char* name,
                                    worker_pref_t pref) {
    if (!pool_ptr || !count_ptr || !entry || min_count == 0)
        return -1;

    worker_pool_descriptor_t* mgr_arg = kmalloc(sizeof(worker_pool_descriptor_t));
    if (!mgr_arg)
        return -1;

    mgr_arg->pool_ptr = pool_ptr;
    mgr_arg->count_ptr = count_ptr;
    mgr_arg->min_count = min_count;
    mgr_arg->grow_by = grow_by;
    mgr_arg->entry = entry;
    mgr_arg->args = args;
    mgr_arg->priority = priority;
    mgr_arg->name = name;
    mgr_arg->pref = pref;

    worker_thread* mgr =
        sched_create_worker_thread(worker_pool_manager_entry, mgr_arg, priority, "worker_pool_mgr", SUBSYSTEM_NONE);
    if (!mgr) {
        kfree(mgr_arg, sizeof(worker_pool_descriptor_t));
        return -1;
    }

    sched_enqueue(sched.ready_queue, mgr->thread);
    return 0;
}

int sched_remove_worker_pool(worker_thread*** pool_ptr, size_t* count_ptr) {
    if (!pool_ptr || !count_ptr)
        return -1;

    worker_thread** pool = *pool_ptr;
    size_t count = *count_ptr;

    for (size_t i = 0; i < count; ++i) {
        worker_thread* w = pool[i];
        if (!w)
            continue;

        sched_remove(sched.kernel_threads, w->thread);

        if (w->thread->kernel_stack) {
            kfree(w->thread->kernel_stack, 4096);
        }
        kfree(w->thread, sizeof(thread_t));
        kfree(w, sizeof(worker_thread));
    }

    kfree(pool, sizeof(worker_thread*) * count);
    *pool_ptr = NULL;
    *count_ptr = 0;
    return 0;
}

int sched_copy_worker_pool(worker_thread*** dest_pool_ptr,
                           size_t* dest_count_ptr,
                           worker_thread** src_pool,
                           size_t src_count) {
    if (!dest_pool_ptr || !dest_count_ptr || !src_pool || src_count == 0)
        return -1;

    worker_thread** dest_pool = kmalloc(sizeof(worker_thread*) * src_count);
    if (!dest_pool)
        return -1;

    size_t copied = 0;
    for (size_t i = 0; i < src_count; ++i) {
        worker_thread* src = src_pool[i];
        if (!src) {
            dest_pool[i] = NULL;
            continue;
        }

        worker_thread* dest = sched_internal_init_worker(
            src->entry, src->arg, src->thread->priority.base, src->thread->name, src->subsystem_pref);
        if (!dest) {
            for (size_t j = 0; j < copied; ++j) {
                worker_thread* cdest = dest_pool[j];
                if (!cdest)
                    continue;

                sched_remove(sched.kernel_threads, cdest->thread);

                if (cdest->thread->kernel_stack) {
                    kfree(cdest->thread->kernel_stack, 4096);
                }
                kfree(cdest->thread, sizeof(thread_t));
                kfree(cdest, sizeof(worker_thread));
            }
            kfree(dest_pool, sizeof(worker_thread*) * src_count);
            return -1;
        }

        sched_thread_list_add(dest->thread, sched.kernel_threads);
        dest_pool[i] = dest;
        copied++;
    }

    // replace dest pool pointer
    worker_thread** old_dest_pool = *dest_pool_ptr;
    size_t old_dest_count = *dest_count_ptr;
    if (old_dest_pool) {
        kfree(old_dest_pool, sizeof(worker_thread*) * old_dest_count);
    }
    *dest_pool_ptr = dest_pool;
    *dest_count_ptr = src_count;
    return 0;
}

void sched_worker_thread_print(void) {
    log("Kernel Worker Threads:\n", YELLOW);
    spinlock(&sched.kernel_threads->lock);
    for (thread_t* t = sched.kernel_threads->head; t; t = t->next) {
        char buffer[128];
        flop_snprintf(buffer, sizeof(buffer), " - %s (priority: %u)\n", t->name, t->priority.base);
        log(buffer, YELLOW);
    }
    spinlock_unlock(&sched.kernel_threads->lock, true);

    log("User Threads:\n", YELLOW);
    spinlock(&sched.user_threads->lock);
    for (thread_t* t = sched.user_threads->head; t; t = t->next) {
        char ubuffer[128];
        flop_snprintf(ubuffer, sizeof(ubuffer), " - %s (priority: %u)\n", t->name, t->priority.base);
        log(ubuffer, YELLOW);
    }
    spinlock_unlock(&sched.user_threads->lock, true);
}

void sched_worker_thread_count_print(void) {
    char buffer[128];
    flop_snprintf(buffer,
                  sizeof(buffer),
                  "Kernel Worker Threads: %u\nUser Threads: %u\n",
                  sched.kernel_threads->count,
                  sched.user_threads->count);
    log(buffer, YELLOW);
}

void sched_worker_pool_print(worker_thread** pool, size_t count) {
    if (!pool || count == 0) {
        log("Worker pool is empty\n", YELLOW);
        return;
    }

    log("Worker Pool Threads:\n", YELLOW);
    for (size_t i = 0; i < count; ++i) {
        worker_thread* w = pool[i];
        if (!w) {
            log(" - NULL\n", YELLOW);
            continue;
        }
        char buffer[128];
        flop_snprintf(buffer, sizeof(buffer), " - %s (priority: %u)\n", w->thread->name, w->thread->priority.base);
        log(buffer, YELLOW);
    }
}

int sched_init_kernel_worker_pool(void) {
    size_t pool_size = 4;
    worker_thread** kernel_worker_pool =
        sched_create_worker_pool(pool_size, NULL, NULL, 5, "kernel_worker", SUBSYSTEM_NONE);
    if (!kernel_worker_pool) {
        log("sched: failed to create kernel worker pool\n", RED);
        return -1;
    }

    log("sched: kernel worker pool created\n", GREEN);
    return 0;
}
