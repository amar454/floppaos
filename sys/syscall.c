#include "../mem/alloc.h"
#include "../mem/pmm.h"
#include "../mem/paging.h"
#include "../mem/vmm.h"
#include "../mem/slab.h"
#include "../mem/utils.h"
#include "../fs/vfs/vfs.h"
#include "../lib/logging.h"
#include "../lib/str.h"
#include "../lib/refcount.h"
#include "../interrupts/interrupts.h"
#include "../task/sync/spinlock.h"
#include "../task/process.h"
#include "../task/sched.h"
#include "../drivers/time/floptime.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "syscall.h"

// fork the current running process
// return pid_t or -1 on failure
pid_t sys_fork(void) {
    process_t* parent = proc_get_current();
    pid_t child_pid = proc_fork(parent);
    if (child_pid < 0) {
        return -1;
    }

    return child_pid;
}

int sys_open(void* path_ptr, uint32_t flags) {
    char* path = (char*) path_ptr;
    if (!path)
        return -1;

    process_t* proc = proc_get_current();

    int fd;
    for (fd = 0; fd < MAX_PROC_FDS; fd++) {
        if (proc->fds[fd].node == NULL) {
            break;
        }
    }

    if (fd == MAX_PROC_FDS) {
        return -1; // no space
    }
    struct vfs_node* file = vfs_open(path, flags);
    if (!file) {
        return -1;
    }
    proc->fds[fd].node = file;
    return fd;
}

int sys_close(int fd) {
    process_t* proc = proc_get_current();

    if (fd < 0 || fd >= MAX_PROC_FDS)
        return -1;

    struct vfs_file_descriptor* desc = &proc->fds[fd];
    if (!desc)
        return -1;

    vfs_close(desc->node);

    kfree(desc, sizeof(struct vfs_file_descriptor));
    proc->fds[fd].node = NULL;
    return 0;
}

int sys_read(int fd, void* buf, size_t count) {
    process_t* proc = proc_get_current();

    if (fd < 0 || fd >= MAX_PROC_FDS) {
        return -1;
    }

    struct vfs_file_descriptor* desc = &proc->fds[fd];
    if (!desc) {
        return -1;
    }
    return vfs_read(desc->node, buf, count);
}

int sys_write(int fd, void* buf, size_t count) {
    process_t* proc = proc_get_current();

    if (fd < 0 || fd >= MAX_PROC_FDS)
        return -1;

    if (!buf)
        return -1;

    struct vfs_file_descriptor* desc = &proc->fds[fd];
    if (!desc || !desc->node)
        return -1;

    return vfs_write(desc->node, buf, count);
}

static void sys_mmap_internal_rb(vmm_region_t* region, uint32_t start, uint32_t end) {
    for (uint32_t a = start; a < end; a += PAGE_SIZE) {
        uintptr_t p = vmm_resolve(region, (uintptr_t) a);
        if (p) {
            vmm_unmap(region, (uintptr_t) a);
            pmm_free_page((void*) p);
        }
    }
}

static int sys_mmap_internal_alloc(vmm_region_t* region, uint32_t vaddr, uint32_t len, uint32_t flags) {
    uint32_t end = vaddr + len;

    for (uint32_t cur = vaddr; cur < end; cur += PAGE_SIZE) {
        void* p = pmm_alloc_page();
        if (!p) {
            sys_mmap_internal_rb(region, vaddr, cur);
            return -1;
        }

        vmm_map(region, cur, (uintptr_t) p, flags);
    }

    return 0;
}

static int sys_mmap_internal_get_va(vmm_region_t* region, uint32_t req, uint32_t len, uint32_t* out) {
    if (!region || !out)
        return -1;

    if (req == 0) {
        uint32_t found = vmm_find_free_range(region, len);
        if (!found)
            return -1;
        *out = found;
        return 0;
    }

    if (req & (PAGE_SIZE - 1))
        return -1;

    *out = req;
    return 0;
}

int sys_mmap(uint32_t addr, uint32_t len, uint32_t flags) {
    if (len == 0) {
        return -1;
    }

    len = ALIGN_UP(len, PAGE_SIZE);

    process_t* proc = proc_get_current();
    vmm_region_t* region = proc->region;
    if (!region) {
        return -1;
    }

    uint32_t vaddr;
    if (sys_mmap_internal_get_va(region, addr, len, &vaddr) < 0) {
        return -1;
    }

    if (sys_mmap_internal_alloc(region, vaddr, len, flags) < 0) {
        return -1;
    }

    return vaddr;
}

void c_syscall_handler() {
    uint32_t num, a1, a2, a3, a4, a5;

    asm volatile("mov %%eax, %0\n"
                 "mov %%ebx, %1\n"
                 "mov %%ecx, %2\n"
                 "mov %%edx, %3\n"
                 "mov %%esi, %4\n"
                 "mov %%edi, %5\n"
                 : "=r"(num), "=r"(a1), "=r"(a2), "=r"(a3), "=r"(a4), "=r"(a5));

    uint32_t ret = -1;

    switch (num) {
        case SYSCALL_READ:
            ret = sys_read(a1, (void*) a2, a3);
            break;
        case SYSCALL_WRITE:
            ret = sys_write(a1, (void*) a2, a3);
            break;
        case SYSCALL_OPEN:
            ret = sys_open((char*) a1, a2);
            break;
        case SYSCALL_CLOSE:
            ret = sys_close(a1);
            break;
        case SYSCALL_FORK:
            ret = sys_fork();
            break;
        case SYSCALL_MMAP:
            ret = sys_mmap(a1, a2, a3);
            break;
    }

    asm volatile("mov %0, %%eax" ::"r"(ret));
}
