#ifndef TASK_HANDLER_H
#define TASK_HANDLER_H

#include <stdint.h>
#define MAX_TASKS 256
// Task function type that can accept an argument (in this case, a FileSystem pointer)
typedef void (*task_fn)(void *);
#define MAX_TASK_NAME_LENGTH 32
#define MAX_SOURCE_PATH_LENGTH 128

extern int task_count;
// Task structure that holds a task and its argument

typedef struct {
    task_fn function;       // Pointer to the task function
    void *arg;              // Arguments for the task
    uint8_t priority;       // Priority of the task (higher is more important)
    uint32_t sleep_ticks;   // Remaining ticks to sleep
    uint32_t runtime;       // Total runtime (in ticks)
    int pid;                // Unique process ID
    void *memory;           // Pointer to allocated memory page
    char name[MAX_TASK_NAME_LENGTH];
    char source_path[MAX_SOURCE_PATH_LENGTH];
} Task;

// Function declarations
void add_task(task_fn task, void *arg, uint8_t priority, const char *name, const char* source_path);
void remove_task_n(int task_index);
void scheduler();                       // Execute the current task and schedule the next one
void initialize_task_system();          // Initialize task system 
void remove_current_task();
void task_sleep(int task_index, uint32_t ticks);
void print_tasks();
#endif // TASK_HANDLER_H