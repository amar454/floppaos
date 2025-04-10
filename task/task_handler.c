#include "task_handler.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../apps/echo.h"
#include "../lib/str.h"
#include "../mem/vmm.h"
#include "../mem/pmm.h"
#include "../mem/paging.h"
#include "../mem/utils.h"
#include "../drivers/vga/vgahandler.h"
#include "../lib/logging.h"
#include "../kernel.h"
#define PAGE_SIZE 4096
#define MAX_VIRTUAL_ADDRESS 0xFFFFF000 // 4GB - 4KB
#define MIN_VIRTUAL_ADDRESS 0x00001000 // Avoid null pointers

uintptr_t global_next_virtual_address = MIN_VIRTUAL_ADDRESS;
int next_pid = 0; // PID generator

// Task queue and related variables
Task task_queue[MAX_TASKS];
int task_count = 0;
int current_task = 0;

void vmm_init_task(Task *task) {
    if (!task) return;

    // Allocate a new page directory for the task
    PDE *page_directory = create_page_directory();
    if (!page_directory) {
        log_step("Failed to allocate page directory!\n", RED);
        return;
    }

    // Assign the page directory to the task
    task->page_directory = page_directory;

    // Assign the next available virtual address to the task
    task->next_virtual_address = global_next_virtual_address;

    // Update the global tracker for the next virtual address
    global_next_virtual_address += PAGE_SIZE;

    log_step("Task virtual memory initialized.\n", GREEN);
}


void *task_alloc_memory(Task *task, uint32_t size) {
    if (!task || size == 0) return NULL;

    uint32_t num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    uintptr_t start_virt_addr = task->next_virtual_address;

    for (uint32_t i = 0; i < num_pages; i++) {
        uintptr_t virt_addr = start_virt_addr + i * PAGE_SIZE;

        if (virt_addr >= MAX_VIRTUAL_ADDRESS) {
            log_step("Virtual address space exhausted for task!\n", RED);
            return NULL;
        }

        void *phys_page = pmm_alloc_page();

        if (!phys_page) {
            log_step("Out of physical memory.\n", RED);

            return NULL;
        }

        vmm_map_page(task->page_directory, virt_addr, (uintptr_t)phys_page, (PageAttributes){
            .present = 1, .rw = 1, .user = 1
        });
    }

    task->next_virtual_address += num_pages * PAGE_SIZE;
    return (void *)start_virt_addr;
}

void remove_current_task() {
    if (task_count == 0) {
        log_step("No tasks to remove!\n", RED);
        return;
    }

    Task *task = &task_queue[current_task];

    // Free the task's virtual memory
    if (task->page_directory) {
        for (uintptr_t addr = MIN_VIRTUAL_ADDRESS; addr < task->next_virtual_address; addr += PAGE_SIZE) {
            uintptr_t phys_addr = vmm_virt_to_phys(task->page_directory, addr);
            if (phys_addr) {
                pmm_free_page((void *)phys_addr);
            }
        }
        pmm_free_page(task->page_directory); // Free page directory
    }

    // Shift tasks to fill the gap
    for (int i = current_task; i < task_count - 1; i++) {
        task_queue[i] = task_queue[i + 1];
    }

    task_count--;

    // Adjust the current_task index
    if (task_count == 0) {
        current_task = 0; // Reset to 0 if no tasks are left
    } else {
        current_task %= task_count; // Ensure current_task is within bounds
    }

    echo("Current task removed successfully.\n", GREEN);
}

void add_task(task_fn task, void *arg, uint8_t priority, const char *name, const char *source_path) {
    if (task_count >= MAX_TASKS) {
        log_step("Task queue is full!\n", RED);
        return;
    }
    log_step("Adding task...\n", LIGHT_GRAY);
    Task *new_task = &task_queue[task_count];
    new_task->function = task;
    new_task->arg = arg;
    new_task->priority = priority;
    new_task->pid = next_pid++;
    new_task->runtime = 0;



    // Initialize the task's virtual memory
    vmm_init_task(new_task);

    // Store the task name and source path
    flopstrncpy(new_task->name, name, MAX_TASK_NAME_LENGTH - 1);
    new_task->name[MAX_TASK_NAME_LENGTH - 1] = '\0';
    flopstrncpy(new_task->source_path, source_path, MAX_SOURCE_PATH_LENGTH - 1);
    new_task->source_path[MAX_SOURCE_PATH_LENGTH - 1] = '\0';
    log_step("Task added successfully. Name: ", GREEN);
    echo(new_task->name, GREEN);
    echo("\n", GREEN);
    task_count++;
}

void remove_task(int task_index) {
    if (task_index < 0 || task_index >= task_count) {
        log_step("Invalid task index!\n", RED);
        return;
    }

    Task *task = &task_queue[task_index];

    // Free the task's virtual memory
    if (task->page_directory) {
        for (uintptr_t addr = MIN_VIRTUAL_ADDRESS; addr < task->next_virtual_address; addr += PAGE_SIZE) {
            uintptr_t phys_addr = vmm_virt_to_phys(task->page_directory, addr);
            if (phys_addr) {
                pmm_free_page((void *)phys_addr);
            }
        }
        pmm_free_page(task->page_directory); // Free page directory
    }

    // Shift tasks to fill the gap
    for (int i = task_index; i < task_count - 1; i++) {
        task_queue[i] = task_queue[i + 1];
    }

    task_count--;
}

// Add these variables for scheduler state
volatile int scheduler_ticks = 0;
volatile bool scheduler_enabled = false;
volatile bool scheduler_paused = false;
volatile int pause_ticks_remaining = 0;

void scheduler() {
    if (task_count == 0 || !scheduler_enabled) return;

    // Increment scheduler ticks
    scheduler_ticks++;

    if (scheduler_paused) {
        if (pause_ticks_remaining > 0) {
            pause_ticks_remaining--;
            if (pause_ticks_remaining == 0) {
                sched_resume();
            }
        }
        return;
    }

    // Only switch tasks every few ticks (time slice)
    const int ticks_per_task = 1;
    if (scheduler_ticks % ticks_per_task != 0) return;
    
    // Update sleep timers for all tasks
    for (int i = 0; i < task_count; i++) {
        if (task_queue[i].sleep_ticks > 0) {
            task_queue[i].sleep_ticks--;
            if (task_queue[i].sleep_ticks == 0) {
                log_f("Task %s (PID: %d) woke up.\n", task_queue[i].name, task_queue[i].pid);
            }
            continue; // Skip running sleeping tasks
        }
    }
    
    // Find next runnable task using round-robin
    int original_task = current_task;
    do {
        current_task = (current_task + 1) % task_count;
        // If we've checked all tasks, just stay on the current one
        if (current_task == original_task) break;
    } while (task_queue[current_task].sleep_ticks > 0);
    
    // Run the selected task
    Task *current = &task_queue[current_task];
    if (current->sleep_ticks == 0) {
        current->runtime++;
        if (current->function) {
            current->function(current->arg);
        }
    }
}



void sched_start() { 
    scheduler_enabled = true;
    echo("Scheduler started.\n", GREEN);
}


void sched_stop() {
    scheduler_enabled = false;
    echo("\nScheduler stopped.\n", YELLOW);
}

void sched_pause(int ticks) {
    scheduler_paused = true;

    pause_ticks_remaining = ticks;
    log_f("Scheduler paused\n", ticks);
}

void sched_pause_seconds(int seconds) {
    sched_pause(seconds * 100);
}

void sched_resume() {
    scheduler_paused = false;
    pause_ticks_remaining = 0;
    echo("Scheduler resumed.\n", GREEN);
    scheduler();

}
void print_tasks() {
    echo("Task Queue:\n", YELLOW);


    // Print column headers
    echo("| PID | Pr | Tks | Run    | Next Virt  | Name   \n", WHITE);

    // Print separator line
    for (int i = 0; i < 47; i++) echo("-", WHITE);
    echo("\n", WHITE);

    // Print each task
    for (int i = 0; i < task_count; i++) {
        char buffer[80];
        flopsnprintf(buffer, sizeof(buffer),
                        "| %3d | %2u | %3u | %6u | 0x%08X | %-6s \n",
                        task_queue[i].pid,
                        task_queue[i].priority,
                        task_queue[i].sleep_ticks,
                        task_queue[i].runtime,
                        task_queue[i].next_virtual_address,
                        task_queue[i].name);
        echo(buffer, WHITE);
    }

    // Print bottom line
    for (int i = 0; i < 47; i++) echo("-", WHITE);
    echo("\n", WHITE);
    }

void task_sleep(int ticks, Task *task) {
    task->sleep_ticks = ticks;
    echo_f("Task %s (PID: %d) is now sleeping for %d ticks.\n", GREEN, task->name, task->pid, ticks);}



void sched_init() {
    task_count = 0;
    current_task = 0;
    scheduler_ticks = 0;
    scheduler_enabled = false;
    scheduler_paused = false;
    log_step("sched_init: Task system initialized.\n", GREEN);
}