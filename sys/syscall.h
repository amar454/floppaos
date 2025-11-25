#ifndef SYSCALL_H
#define SYSCALL_H

void c_syscall_handler(void);

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "../task/process.h"
#include "../apps/echo.h"
#include "../fs/vfs/vfs.h"
#include "../mem/vmm.h"
#include "../mem/pmm.h"
#include "../mem/paging.h"
#include "../mem/alloc.h"
#include "../mem/utils.h"
#include "../mem/slab.h"

typedef enum syscall_num {
    SYSCALL_READ = 0,
    SYSCALL_WRITE = 1,
    SYSCALL_FORK = 2,
    SYSCALL_OPEN = 3,
    SYSCALL_CLOSE = 4,
    SYSCALL_MMAP = 5,
    SYSCALL_SEEK = 6,
    SYSCALL_STAT = 7,
    SYSCALL_FSTAT = 8,
    SYSCALL_UNLINK = 9,
    SYSCALL_MKDIR = 10,
    SYSCALL_RMDIR = 11,
    SYSCALL_TRUNCATE = 12,
    SYSCALL_FTRUNCATE = 13,
    SYSCALL_RENAME = 14,
    SYSCALL_GETPID = 15,
    SYSCALL_CHDIR = 16,
    SYSCALL_DUP = 17,
    SYSCALL_PIPE = 18,
    SYSCALL_CLONE = 19,
    SYSCALL_IOCTL = 20,
    SYSCALL_PRINT = 21,
    SYSCALL_REBOOT = 22,
    SYSCALL_MUNMAP = 23,
    SYSCALL_CREAT = 24,
    SYSCALL_SCHED_YIELD = 25,
    SYSCALL_KILL = 26,
    SYSCALL_LINK = 27,
    SYSCALL_GETUID = 28,
    SYSCALL_GETGID = 29,
    SYSCALL_GETEUID = 30,
    SYSCALL_GETSID = 31,
    SYSCALL_SETUID = 32,
    SYSCALL_SETGID = 33,
    SYSCALL_REGIDT = 34,
    SYSCALL_GET_PRIORITY_MAX = 35,
    SYSCALL_GET_PRIORITY_MIN = 36,
    SYSCALL_FSMOUNT = 37,
    SYSCALL_COPY_FILE_RANGE = 38,
    SYSCALL_GETCWD = 39,
    SYSCALL_MPROTECT = 40,
    SYSCALL_MREMAP = 41
} syscall_num_t;

typedef struct syscall_table {
    int (*sys_read)(int fd, void* buf, size_t count);
    int (*sys_write)(int fd, void* buf, size_t count);
    pid_t (*sys_fork)(void);
    int (*sys_open)(void* path, uint32_t flags);
    int (*sys_close)(int fd);
    int (*sys_mmap)(uintptr_t addr, uint32_t len, uint32_t flags, int fd, uint32_t offset);
    int (*sys_seek)(int fd, int offset, int whence);
    int (*sys_stat)(char* path, stat_t* st);
    int (*sys_fstat)(int fd, stat_t* st);
    int (*sys_unlink)(char* path);
    int (*sys_mkdir)(char* path, uint32_t mode);
    int (*sys_rmdir)(char* path);
    int (*sys_truncate)(char* path, uint64_t length);
    int (*sys_ftruncate)(int fd, uint64_t length);
    int (*sys_rename)(char* oldpath, char* newpath);
    int (*sys_print)(void* str_ptr);
    pid_t (*sys_getpid)(void);
    int (*sys_chdir)(char* path);
    int (*sys_dup)(int fd);
    int (*sys_pipe)(int pipefd[2]);
    pid_t (*sys_clone)(uint32_t flags, void* stack);
    int (*sys_ioctl)(int fd, int request, void* arg);
    int (*sys_reboot)(void);
    int (*sys_munmap)(uintptr_t addr, uint32_t len);
    int (*sys_creat)(char* path, uint32_t mode);
    int (*sys_sched_yield)(void);
    int (*sys_kill)(pid_t pid);
    int (*sys_link)(char* oldpath, char* newpath);
    uid_t (*sys_getuid)(void);
    pid_t (*sys_getgid)(void);
    uid_t (*sys_geteuid)(void);
    pid_t (*sys_getsid)(void);
    int (*sys_setuid)(pid_t ruid, uid_t uid);
    int (*sys_setgid)(pid_t gid, uid_t uid);
    int (*sys_regidt)(pid_t pid, pid_t gid);
    int (*sys_get_priority_max)(void);
    int (*sys_get_priority_min)(void);
    int (*sys_fsmount)(char* source, char* target, int flags);
    int (*sys_copy_file_range)(int fd_in, int fd_out, size_t count);
    struct vfs_node* (*sys_getcwd)(void);
    int (*sys_mprotect)(uintptr_t addr, uint32_t len, uint32_t flags);
    int (*sys_mremap)(uintptr_t addr, uint32_t old_len, uint32_t new_len, uint32_t flags);
} syscall_table_t;

int syscall(syscall_num_t num, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5);

// fork the current running process; returns pid or -1
pid_t sys_fork(void);

// open a file; returns fd or -1
int sys_open(void* path_ptr, uint32_t flags);

// close fd; returns 0 or -1
int sys_close(int fd);

// read from fd; returns bytes read or -1
int sys_read(int fd, void* buf, size_t count);

// write to fd; returns bytes written or -1
int sys_write(int fd, void* buf, size_t count);

// mmap; returns virtual address or -1
int sys_mmap(uintptr_t addr, uint32_t len, uint32_t flags, int fd, uint32_t offset);

// seek to offset
int sys_seek(int fd, int offset, int whence);

// print to console
int sys_print(void* str_ptr);

// stat a file
int sys_stat(char* path, stat_t* st);

// fstat a file descriptor
int sys_fstat(int fd, stat_t* st);

// unlink a file
int sys_unlink(char* path);

// link a file
int sys_link(char* oldpath, char* newpath);

// make directory
int sys_mkdir(char* path, uint32_t mode);

// remove directory
int sys_rmdir(char* path);

// truncate a file
int sys_truncate(char* path, uint64_t length);

// ftruncate a file descriptor
int sys_ftruncate(int fd, uint64_t length);

// rename a file
int sys_rename(char* oldpath, char* newpath);

// get pid of the current running process
pid_t sys_getpid();

// change cwd
int sys_chdir(char* path);

// reboot system
int sys_reboot();

// pipe creation, returns 0 or -1
int sys_pipe(int pipefd[2]);

// create a child process
pid_t sys_clone(uint32_t flags, void* stack);

// ioctl on a file descriptor
int sys_ioctl(int fd, int request, void* arg);

// duplicate a process; returns new pid or -1
pid_t sys_dup(pid_t pid);

// unmap a memory region
int sys_munmap(uintptr_t addr, uint32_t len);

// yield the cpu, allowing other processes to run
int sys_sched_yield(void);

// kill a process by pid, returns 0 or -1
int sys_kill(pid_t pid);

// open or possibly create a file; returns 0 or -1
int sys_creat(char* path, uint32_t mode);

// get user id of current process
uid_t sys_getuid(void);

// get group id of current process
pid_t sys_getgid(void);

// get effective user id of current process
uid_t sys_geteuid(void);

// get session id of current process
pid_t sys_getsid(void);

// set user id of current process
int sys_setuid(pid_t ruid, uid_t uid);

// set group id of current process
int sys_setgid(pid_t rgid, pid_t gid);

// set real and group ids of current process
int sys_regidt(pid_t rgid, pid_t gid);

// mount a filesystem
int sys_fsmount(char* source, char* target, int flags);

// copy file range from one fd to another in 256 byte chunks
int sys_copy_file_range(int fd_in, int fd_out, size_t count);

// get current working directory
struct vfs_node* sys_getcwd(void);

// change memory protection of a region
int sys_mprotect(uintptr_t addr, uint32_t len, uint32_t flags);

// remap a memory region
int sys_mremap(uintptr_t addr, uint32_t old_len, uint32_t new_len, uint32_t flags);

// get max priority value
int sys_get_priority_max(void);

// get min priority value
int sys_get_priority_min(void);

extern syscall_table_t syscall_table;

#endif // SYSCALL_H
