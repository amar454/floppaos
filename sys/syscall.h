#ifndef SYSCALL_H
#define SYSCALL_H

void c_syscall_handler(void);

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "../task/process.h"
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
    SYSCALL_MMAP = 5
} syscall_num_t;

typedef struct syscall_table {
    int (*syscall_read)(int fd, void* buf, size_t count);
    int (*syscall_write)(int fd, void* buf, size_t count);
    pid_t (*syscall_fork)(void);
    int (*syscall_open)(char* path, uint32_t flags);
    int (*syscall_close)(int fd);
    int (*syscall_mmap)(uint32_t addr, uint32_t len, uint32_t flags);
} syscall_table_t;

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
int sys_mmap(uint32_t addr, uint32_t len, uint32_t flags);

int sys_mmap_internal_alloc(vmm_region_t* region, uint32_t vaddr, uint32_t len, uint32_t flags);
int sys_mmap_internal_get_va(vmm_region_t* region, uint32_t req, uint32_t len, uint32_t* out);
void sys_mmap_internal_rb(vmm_region_t* region, uint32_t start, uint32_t end);

#endif // SYSCALL_H
