#ifndef TASK_HANDLER_H
#define TASK_HANDLER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../mem/paging.h"

#define MAX_TASKS 256
#define MAX_TASK_NAME_LENGTH 32
#define MAX_SOURCE_PATH_LENGTH 128
#define PAGE_SIZE 4096
#define MAX_VIRTUAL_ADDRESS 0xFFFFF000 // 4GB - 4KB
#define MIN_VIRTUAL_ADDRESS 0x00001000 // Avoid null pointers

// Function pointer type for task functions
typedef void (*task_fn)(void *arg);

// Structure to define a task
typedef struct Task {
    task_fn function;             // Task function
    void *arg;                    // Arguments for the task
    uint8_t priority;             // Task priority
    uint32_t pid;                 // Process ID
    uint32_t runtime;             // Total runtime
    uint32_t sleep_ticks;         // Remaining sleep ticks
    PDE *page_directory;          // Page directory for the task
    uintptr_t next_virtual_address; // Next available virtual address
    char name[MAX_TASK_NAME_LENGTH]; // Task name
    char source_path[MAX_SOURCE_PATH_LENGTH]; // Source path of the task
} Task;


extern uintptr_t global_next_virtual_address;
extern int next_pid;
extern Task task_queue[MAX_TASKS];
extern int task_count;
extern int current_task;
extern volatile int scheduler_ticks;
extern volatile bool scheduler_enabled;
extern volatile bool scheduler_paused;
extern volatile int pause_ticks_remaining;
// Global task queue
extern Task task_queue[MAX_TASKS];
extern int task_count;
extern int current_task;
void remove_current_task();
// Function prototypes
void vmm_init_task(Task *task);
void *task_alloc_memory(Task *task, uint32_t size);
void add_task(task_fn task, void *arg, uint8_t priority, const char *name, const char *source_path);
void remove_task(int task_index);
void scheduler(void);
void print_tasks(void);
void sched_init();
void sched_pause(int ticks);
void sched_resume();
void sched_start();
void sched_stop();
void task_sleep(int ticks, Task *task) ;
#endif // TASK_HANDLER_H
