#ifndef THREAD_H
#define THREAD_H

#include "sched.h"
#define STACK_SIZE 8192 

thread_t* thread_alloc(void);
void dealloc_thread(thread_t* thread);

thread_t* thread_create(int (*fn)(void*), void* arg);

void thread_kill(thread_t* thread);
void thread_restart(thread_t* t);
thread_t* thread_alloc(void) ;
void dealloc_thread(thread_t* thread);
__attribute__((noreturn)) void thread_exit(void);
thread_t* thread_current(void);
void test_threads(void);
// Debug functions
void debug_thread_entry(void);
void debug_stack_setup(uint32_t stack_top);
void debug_stack_final(uint32_t final_esp);
void debug_context_switch_entry(void);
void debug_context_switch_before_ret(void);
void debug_thread_structure(void);
// Add these to your existing debug functions
void debug_context_switch_got_param(void);
void debug_context_switch_saved_regs(void);
void debug_context_switch_checked_current(void);
void debug_context_switch_loaded_new(void);
// Add these to your debug functions
void debug_context_switch_about_to_load_esp(void);
void debug_print_thread_pointer(uint32_t thread_ptr);
void debug_print_esp_value(uint32_t esp_value);

#endif // THREAD_H
