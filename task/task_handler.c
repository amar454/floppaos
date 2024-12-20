#include "task_handler.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Global variables
Task task_queue[MAX_TASKS];
int task_count = 0;  // Now globally accessible
int current_task = 0;

// Adds a task to the task queue with priority
void add_task(task_fn task, void *arg, uint8_t priority) {
    if (task_count < MAX_TASKS) {
        task_queue[task_count].function = task;
        task_queue[task_count].arg = arg;
        task_queue[task_count].priority = priority;
        task_queue[task_count].sleep_ticks = 0;
        task_count++;
    }
}

// Removes the current task from the queue
void remove_current_task() {
    if (task_count == 0) return; // No tasks to remove

    // Shift tasks down by one to fill the gap created by the removed task
    for (int i = current_task; i < task_count - 1; i++) {
        task_queue[i] = task_queue[i + 1];
    }

    // Decrease the task count and adjust the current task index if necessary
    task_count--;

    // If we removed the last task, wrap around to the first task
    if (current_task >= task_count) {
        current_task = 0;
    }
}

// Removes a task by index
void remove_task_n(int task_index) {
    if (task_index < 0 || task_index >= task_count) return; // Invalid index

    // Shift tasks down by one to fill the gap created by the removed task
    for (int i = task_index; i < task_count - 1; i++) {
        task_queue[i] = task_queue[i + 1];
    }

    // Decrease the task count
    task_count--;

    // If the task removed was after the current task, we don't change the current_task index
    // If the task was the current task, we move to the next task
    if (task_index == current_task) {
        // If we removed the last task, wrap around to the first task
        if (current_task >= task_count) {
            current_task = 0;
        }
    } else if (task_index < current_task) {
        // If the task removed is before the current task, adjust the current_task index
        current_task--;
    }
}

// Round-robin scheduler with priority handling and basic time-slicing
void scheduler() {
    if (task_count == 0) return;

    int ticks_per_task = 5;

    while (1) {
        Task *current = &task_queue[current_task];

        // Skip tasks that are sleeping
        if (current->sleep_ticks > 0) {
            current->sleep_ticks--;
            current_task = (current_task + 1) % task_count; // Move to the next task
            continue;
        }

        // Execute the current task for its time slice
        for (int ticks_remaining = 0; ticks_remaining < ticks_per_task; ticks_remaining++) {
            current->function(current->arg);

            // Reduce sleep timers for all tasks
            for (int i = 0; i < task_count; i++) {
                if (task_queue[i].sleep_ticks > 0) {
                    task_queue[i].sleep_ticks--;
                }
            }

            // Yield to the scheduler after each execution
            if (current->sleep_ticks > 0) break; // Task went to sleep during execution
        }

        // Move to the next task in a round-robin fashion
        current_task = (current_task + 1) % task_count;
    }
}

// Sets a task to sleep for a specified number of ticks
void task_sleep(int task_index, uint32_t ticks) {
    if (task_index >= 0 && task_index < task_count) {
        task_queue[task_index].sleep_ticks = ticks;
    }
}

// Initialize task system
void initialize_task_system() {
    task_count = 0;
    current_task = 0;
}
