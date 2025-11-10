#include "sched.h"
#include "thread.h"
#include "../mem/alloc.h"
#include "../mem/pmm.h"
#include "../mem/paging.h"
#include "../mem/vmm.h"
#include "../mem/utils.h"
#include "../fs/vfs/vfs.h"
#include "../lib/logging.h"
#include "../lib/str.h"
#include "../lib/refcount.h"
#include "../interrupts/interrupts.h"
#include "../task/sync/spinlock.h"
#include "../drivers/time/floptime.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

static bool init_process_ran = false;
static process_t* init_process;
proc_table_t* proc_tbl;
static proc_info_t* proc_info_local;

static void proc_info_init() {
    proc_info_local->next_pid = 1;
    proc_info_local->process_count = 0;
    proc_info_local->table = proc_tbl;
}

void proc_table_init() {
    proc_tbl->processes = NULL;
    static spinlock_t proc_lock_init = SPINLOCK_INIT;
    proc_tbl->proc_table_lock = proc_lock_init;
    spinlock_init(&proc_tbl->proc_table_lock);
}

static void proc_init_process_dummy_entry() {
    if (init_process_ran == false) {
        log("entered init process!", RED);
        init_process_ran = true;
    } else {
        proc_kill(init_process);
    }
}

static void proc_family_init(process_t* process) {
    process->children = NULL;
    process->siblings = NULL;
    process->parent = NULL;
}

// this is kinda tricky because the
// caller of this function is supposed to
// set up the rest of the process_t data structure.
// this does nothing besides allocate and zero the data structures behind a process
static process_t* proc_alloc(void) {
    process_t* process = (process_t*) kmalloc(sizeof(process_t));
    if (!process)
        return NULL;
    flop_memset(process, 0, sizeof(process_t));

    // allocate a thread list for the process
    // threads are allocated as they are created
    // meaning we only need a thread list allocation here
    process->threads = (thread_list_t*) kmalloc(sizeof(thread_list_t));
    if (!process->threads) {
        kfree(process, sizeof(process_t));
        return NULL;
    }

    flop_memset(process->threads, 0, sizeof(thread_list_t));

    for (int i = 0; i < MAX_PROC_FDS; ++i) {
        flop_memset(&process->fds[i], 0, sizeof(struct vfs_file_descriptor));
    }

    process->region = NULL;
    process->mem_usage = 0;
    process->parent = NULL;
    process->children = NULL;
    process->siblings = NULL;
    process->name = NULL;

    process->pid = -1;
    process->sid = -1;
    process->pgid = -1;
    process->rgid = 0;
    process->gid = 0;
    process->ruid = 0;
    process->uid = 0;

    process->state = NULL;
    return process;
}

int proc_cwd_assign(process_t* process, struct vfs_node* cwd) {
    if (!process || !cwd) {
        return -1;
    }

    if (process->cwd) {
        vfs_close(process->cwd);
    }

    process->cwd = cwd;
    refcount_inc_not_zero(&cwd->refcount);
    return 0;
}

int proc_alloc_proc_mem(process_t* process, size_t size) {
    if (!process || size == 0) {
        return -1;
    }

    size_t pages = size / PAGE_SIZE;
    if (size % PAGE_SIZE != 0) {
        pages += 1;
    }
    for (size_t i = 0; i < pages; ++i) {
        if (vmm_alloc(process->region, 1, PAGE_USER | PAGE_RW) == 0) {
            return -1;
        } else {
            process->mem_usage += PAGE_SIZE;
        }
    }
    return 0;
}

int proc_init_process_zero_ids(process_t* process) {
    if (!process) {
        return -1;
    }

    process->pid = 0;
    process->sid = 0;
    process->pgid = 0;
    process->rgid = 0;
    process->gid = 0;
    process->ruid = 0;
    process->uid = 0;

    return 0;
}

int proc_init_process_assign_name(process_t* process, const char* name) {
    if (!process || !name) {
        return -1;
    }

    size_t name_len = flopstrlen(name) + 1;
    process->name = (char*) kmalloc(name_len);
    if (!process->name) {
        return -1;
    }
    flopstrcopy(process->name, name, name_len);
    return 0;
}

int proc_init_process_create_region(process_t* process, size_t initial_pages) {
    if (!process) {
        return -1;
    }

    uintptr_t region_va = 0;
    process->region = vmm_region_create(initial_pages, PAGE_USER | PAGE_RW, &region_va);
    if (!process->region) {
        return -1;
    }
    process->mem_usage = initial_pages * PAGE_SIZE;
    return 0;
}

int proc_init_process_family_create(process_t* init_proc) {
    if (!init_proc || !proc_info_local) {
        return -1;
    }

    init_proc->parent = NULL;
    init_proc->children = NULL;
    init_proc->siblings = NULL;

    init_proc->pid = proc_info_local->next_pid++;
    proc_info_local->process_count++;

    return 0;
}

void proc_init_process_free_data_structures(process_t* process) {
    if (!process) {
        return;
    }

    if (process->cwd) {
        vfs_close(process->cwd);
    }

    if (process->region) {
        vmm_free_region(process->region);
    }

    if (process->name) {
        kfree(process->name, flopstrlen(process->name) + 1);
    }

    if (process->threads) {
        kfree(process->threads, sizeof(thread_list_t));
    }

    kfree(process, sizeof(process_t));
    return;
}

int proc_create_init_process() {
    spinlock(&proc_tbl->proc_table_lock);
    init_process = proc_alloc();
    if (!init_process) {
        spinlock_unlock(&proc_tbl->proc_table_lock, true);
        return -1;
    }

    // processes need to be in the embryo state
    // to follow posix :^)
    init_process->state = EMBRYO;
    spinlock_unlock(&proc_tbl->proc_table_lock, true);

    if (proc_init_process_zero_ids(init_process) < 0) {
        proc_init_process_free_data_structures(init_process);
        return -1;
    }

    // processes do heap allocations for names
    // kmalloc is thread safe so this is ok
    // i'd rather do this than some preallocated buffer
    char* init_process_name = "init_process";
    size_t name_len = flopstrlen(init_process_name) + 1;
    init_process->name = (char*) kmalloc(name_len);
    if (!init_process->name) {
        proc_init_process_free_data_structures(init_process);
        return -1;
    }
    flopstrcopy(init_process->name, init_process_name, name_len);

    // setup the address space
    if (proc_init_process_create_region(init_process, 4) < 0) {
        proc_init_process_free_data_structures(init_process);
        return -1;
    }

    init_process->mem_usage = 4 * PAGE_SIZE;

    if (proc_init_process_family_create(init_process) < 0) {
        vmm_region_destroy(init_process->region);
        proc_init_process_free_data_structures(init_process);
        return -1;
    }

    init_process->cwd = NULL;

    vfs_mount("procfs", "/process/", VFS_TYPE_PROCFS);
    thread_t* init_process_thread =
        sched_create_user_thread(&proc_init_process_dummy_entry, 5, "init_thread", init_process);
    if (init_process_thread) {
        sched_thread_list_add(init_process_thread, init_process->threads);
    } else {
        vmm_region_destroy(init_process->region);
        proc_init_process_free_data_structures(init_process);
        return -1;
    }

    spinlock(&proc_tbl->proc_table_lock);
    init_process->state = RUNNING;
    spinlock_unlock(&proc_tbl->proc_table_lock, true);

    return 0;
}

inline pid_t proc_getpid(process_t* process) {
    if (!process) {
        return -1;
    }
    return process->pid;
}

int proc_kill(process_t* process) {
    if (!process) {
        return -1;
    }

    while (process->threads && process->threads->head) {
        thread_t* thread = process->threads->head;
        sched_remove(sched.ready_queue, thread);
        sched_remove(sched.sleep_queue, thread);
    }

    if (process->cwd) {
        vfs_close(process->cwd);
    }

    vmm_free_region(process->region);
    kfree(process->name, flopstrlen(process->name) + 1);
    kfree(process->threads, sizeof(thread_list_t));
    kfree(process, sizeof(process_t));
    return 0;
}

int proc_stop(process_t* process) {
    if (!process) {
        return -1;
    }

    spinlock(&proc_tbl->proc_table_lock);

    process->state = STOPPED;

    spinlock_unlock(&proc_tbl->proc_table_lock, true);
    return 0;
}

int proc_continue(process_t* process) {
    if (!process) {
        return -1;
    }

    spinlock(&proc_tbl->proc_table_lock);

    if (process->state != STOPPED) {
        spinlock_unlock(&proc_tbl->proc_table_lock, true);
        return -1;
    }
    process->state = RUNNABLE;

    spinlock_unlock(&proc_tbl->proc_table_lock, true);
    return 0;
}

static int proc_copy_fds(process_t* dest, process_t* src) {
    if (!dest || !src) {
        return -1;
    }

    for (int i = 0; i < MAX_PROC_FDS; ++i) {
        flop_memcpy(&dest->fds[i], &src->fds[i], sizeof(struct vfs_file_descriptor));
        if (dest->fds[i].node) {
            refcount_inc_not_zero(&dest->fds[i].node->refcount);
        }
    }
    return 0;
}

static int proc_copy_child_pagemap(process_t* parent, process_t* child) {
    if (!parent || !child) {
        return -1;
    }

    child->region = vmm_copy_pagemap(parent->region);
    if (!child->region) {
        return -1;
    }
    child->mem_usage = parent->mem_usage;
    return 0;
}

static int proc_assign_child_ids(process_t* parent, process_t* child) {
    if (!parent || !child || !proc_info_local) {
        return -1;
    }

    child->pid = proc_info_local->next_pid++;
    child->sid = parent->sid;
    child->pgid = parent->pgid;
    child->rgid = parent->rgid;
    child->gid = parent->gid;
    child->ruid = parent->ruid;
    child->uid = parent->uid;

    proc_info_local->process_count++;

    child->siblings = parent->children;
    parent->children = child;

    return 0;
}

void proc_fork_failed_child_data_structures(process_t* child) {
    if (!child) {
        return;
    }

    if (child->cwd) {
        vfs_close(child->cwd);
    }

    if (child->region) {
        vmm_region_destroy(child->region);
    }

    if (child->name) {
        kfree(child->name, flopstrlen(child->name) + 1);
    }

    if (child->threads) {
        kfree(child->threads, sizeof(thread_list_t));
    }

    kfree(child, sizeof(process_t));
    return;
}

// to be called from sys_fork()
// creates a child process that is a copy of the parent process
// returns the child's pid on success, -1 on failure
pid_t proc_fork(process_t* parent) {
    if (!parent || !proc_info_local || !proc_tbl) {
        return -1;
    }

    process_t* child;

    if (!(child = proc_alloc())) {
        return -1;
    }

    if (proc_copy_child_pagemap(parent, child) < 0) {
        proc_fork_failed_child_data_structures(child);
        return -1;
    }

    if (proc_cwd_assign(child, parent->cwd) < 0) {
        proc_fork_failed_child_data_structures(child);
        return -1;
    }

    if (proc_copy_fds(child, parent) < 0) {
        proc_fork_failed_child_data_structures(child);
        return -1;
    }

    char* child_name = parent->name ? parent->name : "__embryo_process";
    size_t name_len = flopstrlen(child_name) + 1;
    child->name = (char*) kmalloc(name_len);

    if (!child->name) {
        proc_fork_failed_child_data_structures(child);
        return -1;
    }

    flopstrcopy(child->name, child_name, name_len);

    spinlock(&proc_tbl->proc_table_lock);

    if (proc_assign_child_ids(parent, child) < 0) {
        spinlock_unlock(&proc_tbl->proc_table_lock, true);
        proc_fork_failed_child_data_structures(child);
        return -1;
    }
    // back pointer
    child->parent = parent;
    child->state = RUNNABLE;

    spinlock_unlock(&proc_tbl->proc_table_lock, true);

    return child->pid;
}

int proc_init() {
    static proc_info_t proc_info_instance;
    static proc_table_t proc_table_instance;

    proc_info_local = &proc_info_instance;
    proc_tbl = &proc_table_instance;

    proc_info_init();
    proc_table_init();

    if (proc_create_init_process() < 0) {
        log("proc_init: failed to create init process\n", RED);
        return -1;
    }

    log("proc init - ok\n", GREEN);
    return 0;
}