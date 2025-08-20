#ifndef TMPFS_H
#define TMPFS_H

#include "../vfs/vfs.h"
#include "../../lib/refcount.h"
#include <stddef.h>
#include <stdint.h>
#include "../../task/sync/spinlock.h"
typedef struct tmpfs_inode  tmpfs_inode_t;
typedef struct tmpfs_super  tmpfs_super_t;
typedef struct tmpfs_handle tmpfs_handle_t;
typedef struct tmpfs_dirent { tmpfs_inode_t* child; struct tmpfs_dirent* next; } tmpfs_dirent_t;

struct tmpfs_inode {
    char name[VFS_MAX_FILE_NAME];
    int type; 
    tmpfs_inode_t* parent;
    tmpfs_dirent_t* children;
    refcount_t refcount;
    void** pages;        /* array of page pointers */
    size_t page_count;   
    size_t size;   
    spinlock_t lock;       
};

struct tmpfs_super { 
    tmpfs_inode_t* root;
    refcount_t refcount;
    spinlock_t lock;
};

typedef enum {
    TMPFS_CMD_GET_SIZE = 1,
    TMPFS_CMD_SET_SIZE,
    TMPFS_CMD_TRUNCATE,
    TMPFS_CMD_SYNC,
} tmpfs_ctrl_cmd_t;

struct tmpfs_handle { 
    tmpfs_inode_t* inode; 
    size_t pos; 
    int mode;
};

int tmpfs_register_with_vfs();
int tmpfs_init();

#endif // TMPFS_H
