#ifndef TASK_HANDLER_H
#define TASK_HANDLER_H

#include <stdint.h>
#define MAX_TASKS 256
// Task function type that can accept an argument (in this case, a FileSystem pointer)
typedef void (*task_fn)(void *);

// Task structure that holds a task and its argument

typedef struct {
    task_fn function;
    void *arg;
    uint8_t priority;
    uint32_t sleep_ticks;  // Ticks until the task runs again
} Task;

// Function declarations
void add_task(task_fn task, void *arg, uint8_t priority); // Add a task to the task queue with its argument
void scheduler();                       // Execute the current task and schedule the next one
void initialize_task_system();          // Initialize task system (optional)

#endif // TASK_HANDLER_H
