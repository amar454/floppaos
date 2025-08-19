#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>
#include "../../lib/refcount.h"

#define VFS_MAX_FILE_NAME 256

#define VFS_FILE 0x0
#define VFS_DIR 0x1
#define VFS_DEV 0x2
#define VFS_SYMLINK 0x3
#define VFS_HIDDEN 0x4

#define VFS_MODE_R 0x1
#define VFS_MODE_W 0x2
#define VFS_MODE_RW (VFS_MODE_R | VFS_MODE_W)
#define VFS_MODE_CREATE 0x4
#define VFS_MODE_TRUNCATE 0x8
#define VFS_MODE_APPEND 0x9

#define VFS_SEEK_STRT 0x0
#define VFS_SEEK_CUR 0x1
#define VFS_SEEK_END 0x2


struct vfs_mountpoint {
    struct vfs_fs *filesystem;
    struct vfs_mountpoint *next_mountpoint;
    char *mount_point;
    char *device_name;
    void *data_pointer;
    refcount_t refcount;
};

struct vfs_directory_entry {
    char name[256];
    int type; 
    struct vfs_directory_entry* next;
};

struct vfs_directory_list {
    struct vfs_directory_entry* head;
    struct vfs_directory_entry* tail;
};


struct vfs_node {
    struct vfs_mountpoint *mountpoint;
    void* data_pointer;
    int vfs_mode;
    refcount_t refcount;
};


typedef int (*rw) (struct vfs_node*, unsigned char*, unsigned long);

struct vfs_op_tbl {
    struct vfs_node* (*open) (struct vfs_node*, char*); // node, path
    int                (*close)(struct vfs_node*);        // node

    rw  read;
    rw  write;
    
    void*  (*mount)  (char*, char*, int);
    int    (*unmount)(struct vfs_mountpoint*, char*); // mp, device name
    int    (*create) (struct vfs_mountpoint*, char*); // mp, file name
    int    (*delete) (struct vfs_mountpoint*, char*); // mp, file name
    int    (*rename) (struct vfs_mountpoint*, char*, char*); // mp, old name, new name
    int    (*ctrl)   (struct vfs_node*, unsigned long, unsigned long); // node, command, arg
    int    (*seek)   (struct vfs_node*, unsigned long, unsigned char); // node, offset, whence

    struct vfs_directory_list* (*listdir)(struct vfs_mountpoint*, char*); // mp, path


};
struct vfs_fs {
    struct vfs_op_tbl op_table;
    int filesystem_type; 
    struct vfs_fs *previous;
};
int vfs_init(void);

int vfs_acknowledge_fs(struct vfs_fs *fs);
int vfs_unacknowledge_fs(struct vfs_fs *fs);
int vfs_mount(char* device, char *mount_point, int type);
int vfs_unmount(char *mount_point);
struct vfs_node* vfs_open(char* name, int mode);
int vfs_close(struct vfs_node* node);
int vfs_read(struct vfs_node* node, unsigned char* buffer, unsigned long size);
int vfs_write(struct vfs_node* node, unsigned char* buffer, unsigned long size);
struct vfs_directory_list* vfs_listdir(struct vfs_mountpoint *mp, char *path);
int vfs_ctrl(struct vfs_node* node, unsigned long command, unsigned long arg);



    




#endif VFS_H