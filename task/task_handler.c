#include "task_handler.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../apps/echo.h"
#include "../lib/str.h"
#include "../mem/memutils.h"
#include "../drivers/vga/vgahandler.h"
#include "../interrupts/interrupts.h"
int next_pid = 0;  // PID generator
// Global variables
Task task_queue[MAX_TASKS];
int task_count = 0; 
int current_task = 0;
int isFirstTask = 0;

// Adds a task to the task queue with priority and memory allocation
void add_task(task_fn task, void *arg, uint8_t priority, const char *name, const char* source_path) {
    if (task_count < MAX_TASKS) {
        Task *new_task = &task_queue[task_count];
        new_task->function = task;
        new_task->arg = arg;
        new_task->priority = priority;
        new_task->sleep_ticks = 0;
        new_task->pid = next_pid++;  // Assign a unique PID
        new_task->runtime = 0;      // Initialize runtime
        new_task->memory = allocate_page();
        if (!new_task->memory) {
            echo("add_task: Failed to allocate memory page!\n", RED);
            return;
        }
        // Store the task name
        flopstrncpy(new_task->name, name, MAX_TASK_NAME_LENGTH - 1);
        new_task->name[MAX_TASK_NAME_LENGTH - 1] = '\0'; // Ensure null termination

        // Store the source path
        flopstrncpy(new_task->source_path, source_path, MAX_SOURCE_PATH_LENGTH - 1);
        new_task->source_path[MAX_SOURCE_PATH_LENGTH - 1] = '\0'; // Ensure null termination
        task_count++;
    } else {
        echo("add_task: Task queue is full!\n", RED);
    }
}

// Removes the current task from the queue
void remove_current_task() {
    if (task_count == 0) {
        return; // No tasks to remove
    }

    Task *current = &task_queue[current_task];

    // Free the allocated memory page for the task
    if (current->memory != NULL) {
        free_page(current->memory);
        current->memory = NULL;
    }

    // Shift tasks down by one to fill the gap created by the removed task
    for (int i = current_task; i < task_count - 1; i++) {
        task_queue[i] = task_queue[i + 1];
    }

    // Decrease the task count
    task_count--;

    // If the removed task was the last task, wrap around to the first task
    if (current_task >= task_count) {
        current_task = 0;
    }
}

// Removes a task by index
void remove_task_n(int task_index) {
    if (task_index < 0 || task_index >= task_count) {
        echo("remove_task_n: Invalid task index!\n", RED);
        return;
    }

    // Free allocated memory for the task
    free_page(task_queue[task_index].memory);

    // Shift tasks down by one to fill the gap created by the removed task
    for (int i = task_index; i < task_count - 1; i++) {
        task_queue[i] = task_queue[i + 1];
    }

    task_count--;

    // Adjust the current_task index if necessary
    if (task_index <= current_task && current_task > 0) {
        current_task--;
    }
}


// Round-robin scheduler with priority handling and basic time-slicing
void scheduler() {
    //if (task_count == 0 || !schedule_needed) return; // Skip if no tasks or no context switch needed

    //schedule_needed = 0; // Clear the flag

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
// Print detailed task information
void print_tasks() {
    if (task_count == 0) {
        echo("No tasks in the queue.\n", YELLOW);
        return;
    }

    // Display header
    echo("Task Queue:\n", YELLOW);
    echo("+-----+----+-----+--------+------------+--------+\n", WHITE);
    echo("| PID | Pr | Tks | Run    | Mem Addr   | Name   \n", WHITE);
    echo("+-----+----+-----+--------+------------+--------+\n", WHITE);

    // Display each task's details
    for (int i = 0; i < task_count; i++) {
        char buffer[80]; // Ensures it fits within 80 columns
        flopsnprintf(buffer, sizeof(buffer),
                     "| %3d | %2u | %3u | %6u | 0x%08X | %-6s \n",
                     task_queue[i].pid,
                     task_queue[i].priority,
                     task_queue[i].sleep_ticks,
                     task_queue[i].runtime,
                     (uintptr_t)task_queue[i].memory,
                     task_queue[i].name);
        echo(buffer, WHITE);
    }

    // Display footer
    echo("+-----+----+-----+--------+------------+--------+\n", WHITE);
}
 