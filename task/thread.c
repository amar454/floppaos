#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "thread.h"
#include "../mem/alloc.h"
#include "../mem/pmm.h"
#include "../mem/vmm.h"

#include "../lib/logging.h"
#include "../lib/assert.h"
#include "../mem/utils.h"

extern void _thread_entry(void (*fn)(void*), void* arg);
extern uint32_t setup_thread_stack(uint32_t* stack_top, int (*fn)(void*), void* arg);

static uint32_t next_tid = 0;
void debug_stack_setup(uint32_t stack_top) {
    log("DEBUG: setup_thread_stack called with stack_top\n", WHITE);
    if (stack_top == 0) {
        log("DEBUG: ERROR - stack_top is 0!\n", RED);
    }
}

void debug_stack_final(uint32_t final_esp) {
    log("DEBUG: setup_thread_stack final ESP calculated\n", WHITE);
    if (final_esp == 0) {
        log("DEBUG: ERROR - final ESP is 0!\n", RED);
    } else {
        log("DEBUG: final ESP is non-zero\n", WHITE);
    }
}

void debug_context_switch_entry(void) {
    log("DEBUG: context_switch called\n", WHITE);
}

void debug_context_switch_got_param(void) {
    log("DEBUG: context_switch got parameter\n", WHITE);
}

void debug_context_switch_saved_regs(void) {
    log("DEBUG: context_switch saved registers\n", WHITE);
}

void debug_context_switch_checked_current(void) {
    log("DEBUG: context_switch checked current thread\n", WHITE);
}

void debug_context_switch_loaded_new(void) {
    log("DEBUG: context_switch loaded new ESP\n", WHITE);
}

void debug_context_switch_before_ret(void) {
    log("DEBUG: context_switch about to ret\n", WHITE);
}
void debug_context_switch_about_to_load_esp(void) {
    log("DEBUG: context_switch about to load ESP\n", WHITE);
}

void debug_print_thread_pointer(uint32_t thread_ptr) {
    log("DEBUG: thread pointer: ", WHITE);
    log_uint("", thread_ptr);
}

void debug_print_esp_value(uint32_t esp_value) {
    log("DEBUG: ESP value to load: ", WHITE);
    log_uint("", esp_value);
    if (esp_value == 0) {
        log("ESP IS ZERO - THIS IS THE PROBLEM!\n", RED);
    } else {
        log("ESP is non-zero\n", WHITE);
    }
}

thread_t* thread_alloc(void) {
    log("thread_alloc: allocating thread_t\n", WHITE);
    
    // Debug: Check if kmalloc is working
    log("thread_alloc: calling kmalloc\n", WHITE);
    thread_t* t;

    t = (thread_t*) vmm_malloc(sizeof(thread_t));

    if (!t) {
        log("thread_alloc: Failed to allocate thread_t\n", WHITE);
        return NULL;
    }
    t->id = next_tid++;
    log("thread_alloc: assigned thread id\n", WHITE);
    
    // Test if the ID assignment worked
    if (t->id == 0 && next_tid > 1) {
        log("thread_alloc: ERROR - ID assignment failed!\n", RED);
        return NULL;
    }
    
    t->node.next = NULL;
    t->node.prev = NULL;
    t->state = THREAD_READY;
    t->esp = 0;
    t->base_esp = 0;
    
    log("thread_alloc: thread allocation successful\n", WHITE);
    return t;
}

void dealloc_thread(thread_t* thread) {
    log("dealloc_thread: deallocating thread\n", WHITE);
    if (thread->base_esp) {
        log("dealloc_thread: freeing stack\n", WHITE);
        kfree((void*)thread->base_esp, STACK_SIZE);
    }
    kfree(thread, sizeof(thread_t));
}

thread_t* thread_create(int (*fn)(void*), void* arg) {
    log("thread_create: creating new thread\n", WHITE);
    thread_t* t = thread_alloc();
    if (!t) {
        return NULL;
    }

    uint32_t* stack = (uint32_t*)vmm_malloc(STACK_SIZE);
    if (!stack) {
        log("thread_create: Failed to allocate stack\n", WHITE);
        dealloc_thread(t);
        return NULL;
    }
    log("thread_create: allocated stack\n", WHITE);
    uint32_t* stack_top = stack + STACK_SIZE / sizeof(uint32_t);

    t->base_esp = (uintptr_t)stack;

    log("thread_create: calling setup_thread_stack\n", WHITE);
    uint32_t new_esp = setup_thread_stack(stack_top, fn, arg);
    log("thread_create: setup_thread_stack returned\n", WHITE);
    if (new_esp == 0) {
        log("thread_create: ERROR - setup_thread_stack returned 0!\n", RED);
        return NULL;
    } else {
        log("thread_create: setup_thread_stack returned non-zero ESP\n", WHITE);
    }
    log("thread_create: testing ESP field directly\n", WHITE);

    volatile uint32_t* esp_ptr = &(t->esp);

    *esp_ptr = 0xDEADBEEF;

    uint32_t test_read = *esp_ptr;
    if (test_read != 0xDEADBEEF) {
        log("thread_create: ERROR - ESP field is not writable!\n", RED);
        return NULL;
    }
    
    log("thread_create: ESP field test passed\n", WHITE);
    
    *esp_ptr = new_esp;
    

    uint32_t readback = *esp_ptr;
    if (readback == 0) {
        log("thread_create: ERROR - ESP became 0 immediately after write!\n", RED);
        

        *esp_ptr = new_esp;
        readback = *esp_ptr;
        if (readback == 0) {
            log("thread_create: ERROR - ESP still 0 after second write!\n", RED);
        } else {
            log("thread_create: ESP worked on second try\n", WHITE);
        }
    } else if (readback != new_esp) {
        log("thread_create: ERROR - ESP value corrupted during write!\n", RED);
    } else {
        log("thread_create: ESP stored correctly\n", WHITE);
    }
    if (t->esp == 0) {
        log("thread_create: ERROR - t->esp is 0 but direct pointer worked!\n", RED);
    }
    
    t->state = THREAD_READY;
    log("thread_create: enqueuing thread\n", WHITE);
    thread_enqueue(t);

    return t;
}

void thread_kill(thread_t* thread) {
    log("thread_kill: killing thread\n", WHITE);
    thread->state = THREAD_DEAD;
    thread_dequeue(thread);
}

void thread_restart(thread_t* t) {
    log("thread_restart: restarting thread\n", WHITE);
    if (t->state == THREAD_EXITED || t->state == THREAD_DEAD) {
        t->state = THREAD_READY;
        thread_enqueue(t);
    }
}

__attribute__((noreturn)) void thread_exit(void) {
    log("thread_exit: current thread exiting\n", WHITE);
    thread_t* self = thread_current();
    self->state = THREAD_EXITED;
    yield();
    __builtin_unreachable();
}

thread_t* thread_current(void) {
    extern thread_t* current_thread;
    log("thread_current: fetching current thread\n", WHITE);
    return (thread_t*)current_thread;
}

int thread_func1(void* arg) {
    log("thread_func1: hello from thread 1\n", WHITE);
    for(int i = 0; i < 5; ++i) {
        log("thread_func1: yielding\n", WHITE);
        yield();
    }
    log("thread_func1: exiting\n", WHITE);
    return 0;
}

int thread_func2(void* arg) {
    log("thread_func2: hello from thread 2\n", WHITE);
    for(int i = 0; i < 5; ++i) {
        log("thread_func2: yielding\n", WHITE);
        yield();
    }
    log("thread_func2: exiting\n", WHITE);
    return 0;
}

void test_threads(void) {
    log("test_threads: Starting thread test\n", WHITE);
    thread_t* t1 = thread_create(thread_func1, NULL);
    log("test_threads: created thread 1\n", WHITE);
    log("test_threads: yielding from main\n", WHITE);
    yield();
    log("test_threads: returned from yield\n", WHITE);
}
