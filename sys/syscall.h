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
    SYSCALL_DUP = 17
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

// pipe creation
int sys_pipe(int pipefd[2]);

// create a child process
pid_t sys_clone(uint32_t flags, void* stack);

// ioctl on a file descriptor
int sys_ioctl(int fd, int request, void* arg);

// duplicate a process; returns new pid or -1
pid_t sys_dup(pid_t pid);

extern syscall_table_t syscall_table;

#endif // SYSCALL_H
