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
#include "../drivers/acpi/acpi.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "syscall.h"

syscall_table_t syscall_table = {.sys_read = sys_read,
                                 .sys_write = sys_write,
                                 .sys_fork = sys_fork,
                                 .sys_open = sys_open,
                                 .sys_close = sys_close,
                                 .sys_mmap = sys_mmap,
                                 .sys_seek = sys_seek,
                                 .sys_stat = sys_stat,
                                 .sys_fstat = sys_fstat,
                                 .sys_unlink = sys_unlink,
                                 .sys_mkdir = sys_mkdir,
                                 .sys_rmdir = sys_rmdir,
                                 .sys_truncate = sys_truncate,
                                 .sys_ftruncate = sys_ftruncate,
                                 .sys_rename = sys_rename,
                                 .sys_print = sys_print,
                                 .sys_getpid = sys_getpid,
                                 .sys_chdir = sys_chdir,
                                 .sys_dup = sys_dup,
                                 .sys_clone = sys_clone,
                                 .sys_pipe = sys_pipe,
                                 .sys_ioctl = sys_ioctl,
                                 .sys_reboot = sys_reboot,
                                 .sys_munmap = sys_munmap,
                                 .sys_creat = sys_creat,
                                 .sys_sched_yield = sys_sched_yield,
                                 .sys_kill = sys_kill,
                                 .sys_link = sys_link,
                                 .sys_getuid = sys_getuid,
                                 .sys_getgid = sys_getgid,
                                 .sys_geteuid = sys_geteuid,
                                 .sys_getsid = sys_getsid,
                                 .sys_setuid = sys_setuid,
                                 .sys_setgid = sys_setgid,
                                 .sys_regidt = sys_regidt,
                                 .sys_get_priority_max = sys_get_priority_max,
                                 .sys_get_priority_min = sys_get_priority_min,
                                 .sys_fsmount = sys_fsmount,
                                 .sys_copy_file_range = sys_copy_file_range};

int syscall(syscall_num_t num, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5) {
    uint32_t ret;

    register uint32_t r_eax asm("eax") = num;
    register uint32_t r_ebx asm("ebx") = a1;
    register uint32_t r_ecx asm("ecx") = a2;
    register uint32_t r_edx asm("edx") = a3;
    register uint32_t r_esi asm("esi") = a4;
    register uint32_t r_edi asm("edi") = a5;

    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(r_eax), "b"(r_ebx), "c"(r_ecx), "d"(r_edx), "S"(r_esi), "D"(r_edi)
                     : "memory");

    return ret;
}

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

pid_t sys_dup(pid_t pid) {
    return proc_dup(pid);
}

int sys_open(void* path, uint32_t flags) {
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

    if (fd < 0 || fd >= MAX_PROC_FDS) {
        return -1;
    }

    struct vfs_file_descriptor* desc = &proc->fds[fd];

    if (!desc) {
        return -1;
    }

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

int sys_copy_file_range(int fd_in, int fd_out, size_t count) {
    process_t* proc = proc_get_current();

    if (fd_in < 0 || fd_in >= MAX_PROC_FDS || fd_out < 0 || fd_out >= MAX_PROC_FDS) {
        return -1;
    }

    struct vfs_file_descriptor* src = &proc->fds[fd_in];
    struct vfs_file_descriptor* dst = &proc->fds[fd_out];

    if (!src || !dst || !src->node || !dst->node) {
        return -1;
    }

    size_t chunk = 256;
    if (count < chunk)
        chunk = count;

    unsigned char* buffer = kmalloc(chunk);
    if (!buffer)
        return -1;

    size_t total = 0;
    while (total < count) {
        size_t to_read = count - total;
        if (to_read > chunk)
            to_read = chunk;

        int r = vfs_read(src->node, buffer, to_read);
        if (r <= 0)
            break;

        int w = vfs_write(dst->node, buffer, r);
        if (w <= 0)
            break;

        total += w;

        if ((size_t) r < to_read)
            break;
    }

    kfree(buffer, sizeof(char) * chunk);
    return total;
}

int sys_seek(int fd, int offset, int whence) {
    process_t* proc = proc_get_current();

    if (fd < 0 || fd >= MAX_PROC_FDS) {
        return -1;
    }

    struct vfs_file_descriptor* desc = &proc->fds[fd];
    if (!desc || !desc->node) {
        return -1;
    }

    return vfs_seek(desc->node, offset, whence);
}

int sys_write(int fd, void* buf, size_t count) {
    process_t* proc = proc_get_current();

    if (fd < 0 || fd >= MAX_PROC_FDS)
        return -1;

    if (!buf)
        return -1;

    if (fd == 1) {
        echo(buf, WHITE);
        return 0;
    }

    struct vfs_file_descriptor* desc = &proc->fds[fd];
    if (!desc || !desc->node)
        return -1;

    return vfs_write(desc->node, buf, count);
}

int sys_print(void* str_ptr) {
    if (!str_ptr)
        return -1;

    char* s = (char*) str_ptr;
    size_t len = flopstrlen(s);
    if (len == 0)
        return 0;

    // this calls echo, the kernel printing backend
    return sys_write(1, s, len);
}

static void sys_mmap_internal_rb(vmm_region_t* region, uintptr_t start_va, uintptr_t end_va) {
    for (uintptr_t va = start_va; va < end_va; va += PAGE_SIZE) {
        uintptr_t phys_addr = vmm_resolve(region, va);
        if (phys_addr) {
            vmm_unmap(region, va);
            pmm_free_page((void*) phys_addr);
        }
    }
}

static int sys_mmap_internal_alloc(
    vmm_region_t* region, uintptr_t base_vaddr, uint32_t length, uint32_t flags, struct vfs_node* node) {
    uintptr_t end_vaddr = base_vaddr + length;

    for (uintptr_t cur_vaddr = base_vaddr; cur_vaddr < end_vaddr; cur_vaddr += PAGE_SIZE) {
        void* phys_page = pmm_alloc_page();
        if (!phys_page) {
            sys_mmap_internal_rb(region, base_vaddr, cur_vaddr);
            return -1;
        }

        if (node) {
            int read_bytes = vfs_read(node, phys_page, PAGE_SIZE);
            if (read_bytes < 0) {
                uint8_t* bp = (uint8_t*) phys_page;
                for (size_t i = 0; i < PAGE_SIZE; i++)
                    bp[i] = 0;
            } else if ((size_t) read_bytes < PAGE_SIZE) {
                uint8_t* bp = (uint8_t*) phys_page;
                for (size_t i = read_bytes; i < PAGE_SIZE; i++)
                    bp[i] = 0;
            }
        } else {
            uint8_t* bp = (uint8_t*) phys_page;
            for (size_t i = 0; i < PAGE_SIZE; i++)
                bp[i] = 0;
        }

        vmm_map(region, cur_vaddr, (uintptr_t) phys_page, flags);
    }

    return 0;
}

static int sys_mmap_internal_get_va(vmm_region_t* region, uint32_t requested_va, uint32_t length, uint32_t* out_va) {
    if (!region || !out_va)
        return -1;

    if (requested_va == 0) {
        uint32_t found_va = vmm_find_free_range(region, length);
        if (!found_va)
            return -1;
        *out_va = found_va;
        return 0;
    }

    if (requested_va & (PAGE_SIZE - 1))
        return -1;

    *out_va = requested_va;
    return 0;
}

int sys_mmap_internal_find_and_seek(int fd, struct vfs_node** out_node, uint32_t offset) {
    process_t* proc = proc_get_current();

    if (fd < 0 || fd >= MAX_PROC_FDS) {
        return -1;
    }

    struct vfs_file_descriptor* descriptor = &proc->fds[fd];

    if (!descriptor || !descriptor->node) {
        return -1;
    }

    *out_node = descriptor->node;

    if (vfs_seek(*out_node, offset, 0) < 0) {
        return -1;
    }

    return 0;
}

int sys_mmap(uintptr_t addr, uint32_t len, uint32_t flags, int fd, uint32_t offset) {
    if (len == 0) {
        return -1;
    }

    len = ALIGN_UP(len, PAGE_SIZE);

    process_t* proc = proc_get_current();
    vmm_region_t* region = proc->region;

    if (!region) {
        return -1;
    }

    struct vfs_node* node = NULL;
    if (fd >= 0) {
        if (sys_mmap_internal_find_and_seek(fd, &node, offset) < 0) {
            return -1;
        }
    }

    uintptr_t map_start_va;
    if (sys_mmap_internal_get_va(region, addr, len, (uint32_t*) &map_start_va) < 0) {
        return -1;
    }

    if (sys_mmap_internal_alloc(region, map_start_va, len, flags, node) < 0) {
        return -1;
    }

    return map_start_va;
}

static int sys_munmap_internal_validate(vmm_region_t* region, uintptr_t addr, uint32_t len) {
    if (!region)
        return -1;

    if (len == 0)
        return -1;

    if (addr & (PAGE_SIZE - 1))
        return -1;

    len = ALIGN_UP(len, PAGE_SIZE);

    return 0;
}

static void sys_munmap_internal_free_phys(vmm_region_t* region, uintptr_t start_va, uintptr_t end_va) {
    for (uintptr_t va = start_va; va < end_va; va += PAGE_SIZE) {
        uintptr_t phys = vmm_resolve(region, va);
        if (phys) {
            vmm_unmap(region, va);
            pmm_free_page((void*) phys);
        }
    }
}

static int sys_munmap_internal_unmap_range(vmm_region_t* region, uintptr_t addr, uint32_t len) {
    uintptr_t end = addr + len;

    for (uintptr_t va = addr; va < end; va += PAGE_SIZE) {
        uintptr_t phys = vmm_resolve(region, va);
        if (!phys) {
            // not mapped
            return -1;
        }
    }

    sys_munmap_internal_free_phys(region, addr, end);
    return 0;
}

static int sys_munmap_internal(process_t* proc, uintptr_t addr, uint32_t len) {
    if (!proc || !proc->region)
        return -1;

    vmm_region_t* region = proc->region;

    if (sys_munmap_internal_validate(region, addr, len) < 0)
        return -1;

    len = ALIGN_UP(len, PAGE_SIZE);

    return sys_munmap_internal_unmap_range(region, addr, len);
}

int sys_munmap(uintptr_t addr, uint32_t len) {
    process_t* proc = proc_get_current();
    if (!proc)
        return -1;

    return sys_munmap_internal(proc, addr, len);
}

int sys_stat(char* path, stat_t* st) {
    if (!path || !st)
        return -1;

    return vfs_stat(path, st);
}

int sys_fstat(int fd, stat_t* st) {
    process_t* proc = proc_get_current();

    if (fd < 0 || fd >= MAX_PROC_FDS)
        return -1;

    struct vfs_file_descriptor* d = &proc->fds[fd];
    if (!d || !d->node)
        return -1;

    return vfs_fstat(d->node, st);
}

int sys_unlink(char* path) {
    if (!path)
        return -1;
    return vfs_unlink(path);
}

int sys_mkdir(char* path, uint32_t mode) {
    if (!path)
        return -1;
    return vfs_mkdir(path, mode);
}

int sys_rmdir(char* path) {
    if (!path)
        return -1;
    return vfs_rmdir(path);
}

int sys_truncate(char* path, uint64_t length) {
    if (!path)
        return -1;
    return vfs_truncate_path(path, length);
}

int sys_ftruncate(int fd, uint64_t length) {
    process_t* proc = proc_get_current();
    if (fd < 0 || fd >= MAX_PROC_FDS)
        return -1;

    struct vfs_file_descriptor* desc = &proc->fds[fd];
    if (!desc || !desc->node)
        return -1;

    return vfs_ftruncate(desc->node, length);
}

int sys_rename(char* oldpath, char* newpath) {
    if (!oldpath || !newpath)
        return -1;
    return vfs_rename(oldpath, newpath);
}

pid_t sys_getpid() {
    process_t* proc = proc_get_current();
    if (!proc) {
        return -1;
    }
    return proc->pid;
}

uid_t sys_getuid() {
    process_t* proc = proc_get_current();
    if (!proc) {
        return (uid_t) -1;
    }
    return proc->uid;
}

pid_t sys_getgid() {
    process_t* proc = proc_get_current();
    if (!proc) {
        return (uid_t) -1;
    }
    return proc->gid;
}

uid_t sys_geteuid() {
    process_t* proc = proc_get_current();
    if (!proc) {
        return (uid_t) -1;
    }
    return proc->ruid;
}

pid_t sys_getsid() {
    process_t* proc = proc_get_current();
    if (!proc) {
        return (uid_t) -1;
    }
    return proc->sid;
}

int sys_setsid(pid_t sid) {
    process_t* proc = proc_get_current();
    if (!proc) {
        return -1;
    }

    proc->sid = sid;
    return 0;
}

int sys_regidt(pid_t rgid, pid_t gid) {
    process_t* proc = proc_get_current();
    if (!proc) {
        return -1;
    }

    if (rgid != (pid_t) -1) {
        proc->rgid = rgid;
    }

    if (gid != (pid_t) -1) {
        proc->gid = gid;
    }

    return 0;
}

int sys_setuid(pid_t ruid, uid_t uid) {
    process_t* proc = proc_get_current();
    if (!proc) {
        return -1;
    }

    if (ruid != (pid_t) -1) {
        proc->ruid = ruid;
    }

    if (uid != (pid_t) -1) {
        proc->uid = uid;
    }

    return 0;
}

int sys_setgid(pid_t rgid, pid_t gid) {
    process_t* proc = proc_get_current();
    if (!proc) {
        return -1;
    }

    if (rgid != (pid_t) -1) {
        proc->rgid = rgid;
    }

    if (gid != (pid_t) -1) {
        proc->gid = gid;
    }

    return 0;
}

pid_t sys_chdir(char* path) {
    if (!path)
        return -1;

    process_t* proc = proc_get_current();
    if (!proc)
        return -1;

    struct vfs_node* new_directory = vfs_open(path, 0);
    if (!new_directory)
        return -1;

    if (proc->cwd)
        vfs_close(proc->cwd);

    proc->cwd = new_directory;
    return 0;
}

int sys_reboot() {
    qemu_power_off();
    return 0;
}

int sys_pipe(int pipefd[2]) {
    if (!pipefd)
        return -1;

    pipe_t* p = kmalloc(sizeof(pipe_t));
    if (!p)
        return -1;

    pipe_init(p);

    process_t* proc = proc_get_current();
    if (!proc) {
        kfree(p, sizeof(pipe_t));
        return -1;
    }

    int read_fd = -1, write_fd = -1;
    for (int i = 0; i < MAX_PROC_FDS; i++) {
        if (proc->fds[i].node == NULL) {
            if (read_fd < 0) {
                read_fd = i;
            } else if (write_fd < 0) {
                write_fd = i;
                break;
            }
        }
    }

    if (read_fd < 0 || write_fd < 0) {
        kfree(p, sizeof(pipe_t));
        return -1;
    }

    proc->fds[read_fd].pipe = p;
    proc->fds[write_fd].pipe = p;

    pipefd[0] = read_fd;
    pipefd[1] = write_fd;

    return 0;
}

pid_t sys_clone(uint32_t flags, void* stack) {
    (void) flags;
    (void) stack;

    process_t* parent = proc_get_current();
    if (!parent)
        return -1;

    pid_t child_pid = proc_fork(parent);
    if (child_pid < 0)
        return -1;

    return child_pid;
}

int sys_ioctl(int fd, int request, void* arg) {
    process_t* proc = proc_get_current();
    if (!proc || fd < 0 || fd >= MAX_PROC_FDS)
        return -1;

    struct vfs_file_descriptor* desc = &proc->fds[fd];
    if (!desc || !desc->node)
        return -1;

    if (desc->node->ops->ioctl)
        return desc->node->ops->ioctl(desc->node, request, (unsigned long) arg);

    return -1;
}

int sys_sched_yield() {
    sched_yield();
    return 0;
}

extern proc_table_t* proc_tbl;

int sys_kill(pid_t pid) {
    process_t* proc = proc_get_current();
    if (!proc)
        return -1;

    process_t* target_proc = proc_get_process_by_pid(pid);
    if (!target_proc)
        return -1;

    if (target_proc->parent != proc)
        return -1;

    spinlock(&proc_tbl->proc_table_lock);

    target_proc->state = TERMINATED;

    spinlock_unlock(&proc_tbl->proc_table_lock, true);
    return 0;
}

int sys_creat(char* path, uint32_t mode) {
    if (!path)
        return -1;

    return vfs_open(path, VFS_MODE_CREATE | VFS_MODE_TRUNCATE | VFS_MODE_RW | mode) ? 0 : -1;
}

int sys_link(char* oldpath, char* newpath) {
    if (!oldpath || !newpath)
        return -1;

    return vfs_link(oldpath, newpath);
}

int sys_get_priority_max() {
    return MAX_PRIORITY;
}

int sys_get_priority_min() {
    return 0;
}

int sys_fsmount(char* source, char* target, int flags) {
    return vfs_mount(source, target, flags);
}

int sys_exit_group(int status) {
    process_t* proc = proc_get_current();
    if (!proc)
        return -1;

    for (int fd = 0; fd < MAX_PROC_FDS; fd++) {
        if (proc->fds[fd].node) {
            sys_close(fd);
        }
    }

    proc_exit_all_threads(proc);
    return 0;
}

void c_syscall_routine() {
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
            ret = sys_mmap(a1, a2, a3, a4, a5);
            break;
        case SYSCALL_STAT:
            ret = sys_stat((char*) a1, (stat_t*) a2);
            break;
        case SYSCALL_FSTAT:
            ret = sys_fstat(a1, (stat_t*) a2);
            break;
        case SYSCALL_UNLINK:
            ret = sys_unlink((char*) a1);
            break;
        case SYSCALL_MKDIR:
            ret = sys_mkdir((char*) a1, a2);
            break;
        case SYSCALL_RMDIR:
            ret = sys_rmdir((char*) a1);
            break;
        case SYSCALL_TRUNCATE:
            ret = sys_truncate((char*) a1, a2);
            break;
        case SYSCALL_FTRUNCATE:
            ret = sys_ftruncate(a1, a2);
            break;
        case SYSCALL_RENAME:
            ret = sys_rename((char*) a1, (char*) a2);
            break;
        case SYSCALL_GETPID:
            ret = sys_getpid();
            break;
        case SYSCALL_CHDIR:
            ret = sys_chdir((char*) a1);
            break;
        case SYSCALL_DUP:
            ret = sys_dup(a1);
            break;
        case SYSCALL_PIPE:
            ret = sys_pipe((int*) a1);
            break;
        case SYSCALL_CLONE:
            ret = sys_clone(a1, (void*) a2);
            break;
        case SYSCALL_IOCTL:
            ret = sys_ioctl(a1, a2, (void*) a3);
            break;
        case SYSCALL_PRINT:
            ret = sys_print((void*) a1);
            break;
        case SYSCALL_REBOOT:
            ret = sys_reboot();
            break;
        case SYSCALL_SEEK:
            ret = sys_seek(a1, a2, a3);
            break;
        case SYSCALL_MUNMAP:
            ret = sys_munmap(a1, a2);
            break;
        case SYSCALL_CREAT:
            ret = sys_creat((char*) a1, a2);
            break;
        case SYSCALL_SCHED_YIELD:
            ret = sys_sched_yield();
            break;
        case SYSCALL_KILL:
            ret = sys_kill(a1);
            break;
        case SYSCALL_LINK:
            ret = sys_link((char*) a1, (char*) a2);
            break;
        case SYSCALL_GETUID:
            ret = sys_getuid();
            break;
        case SYSCALL_GETGID:
            ret = sys_getgid();
            break;
        case SYSCALL_GETEUID:
            ret = sys_geteuid();
            break;
        case SYSCALL_GETSID:
            ret = sys_getsid();
            break;
        case SYSCALL_SETUID:
            ret = sys_setuid((pid_t) a1, (uid_t) a2);
            break;
        case SYSCALL_SETGID:
            ret = sys_setgid((pid_t) a1, (pid_t) a2);
            break;
        case SYSCALL_REGIDT:
            ret = sys_regidt((pid_t) a1, (pid_t) a2);
            break;
        case SYSCALL_GET_PRIORITY_MAX:
            ret = sys_get_priority_max();
            break;
        case SYSCALL_GET_PRIORITY_MIN:
            ret = sys_get_priority_min();
            break;
        case SYSCALL_FSMOUNT:
            ret = sys_fsmount((char*) a1, (char*) a2, (int) a3);
            break;
        case SYSCALL_COPY_FILE_RANGE:
            ret = sys_copy_file_range((int) a1, (int) a2, (size_t) a3);
            break;
        default:
            log("c_syscall_routine: Unknown syscall number\n", RED);
            break;
    }

    asm volatile("mov %0, %%eax" ::"r"(ret));
}
