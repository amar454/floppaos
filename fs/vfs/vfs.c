#include "vfs.h"
#include "../../lib/logging.h"
#include "../../lib/refcount.h"
#include "../../mem/alloc.h"
#include "../../mem/paging.h"
#include "../../mem/vmm.h"
#include "../../drivers/vga/vgahandler.h"
#include "../../lib/str.h"
#include "../../task/sync/spinlock.h"
#include <stddef.h>
#include <stdint.h>

struct vfs_fs_list {
    struct vfs_fs *head;
};

struct vfs_mp_list {
    struct vfs_mountpoint *head;
    struct vfs_mountpoint *tail;
    spinlock_t lock;
};
struct vfs_fs_list fs_list = { 
    .head = NULL 
};

struct vfs_mp_list mp_list = { 
    .head = NULL, 
    .tail = NULL,
    .lock = SPINLOCK_INIT
};



int vfs_init(void) {
    static spinlock_t initializer = SPINLOCK_INIT;
    mp_list.lock = initializer;
    spinlock_init(&mp_list.lock);


    return 0;
}

int vfs_acknowledge_fs(struct vfs_fs *fs) {


    if (!fs) {
        log("vfs_acknowledge_fs: fs is NULL\n", RED);
        return -1;
    }
    fs->previous = fs_list.head;
    fs_list.head = fs;

    log_uint("vfs: acknowledged filesystem type ", fs->filesystem_type);
    return 0;
}


int vfs_unacknowledge_fs(struct vfs_fs *fs) {
    return 0; 
}

struct vfs_fs *vfs_find_fs(int type) {
    struct vfs_fs *fs_to_find;
    for (fs_to_find = fs_list.head; fs_to_find != NULL; fs_to_find = fs_to_find->previous) {
        if (fs_to_find->filesystem_type == type) {
            break;
        }
    }
    return fs_to_find;
}

struct vfs_mountpoint *vfs_file_to_mountpoint(char* name) {
    struct vfs_mountpoint *mp;


    for (mp = mp_list.tail; mp != NULL; mp = mp->next_mountpoint) {
        if (flopstrncmp(mp->mount_point, name, flopstrlen(mp->mount_point)) == 0) {
            break;
        }
    }
    return mp;
}



static struct vfs_mountpoint* vfs_mp_struc_alloc(void) {
    struct vfs_mountpoint* mp = (struct vfs_mountpoint*)kmalloc(sizeof(struct vfs_mountpoint));
    if (!mp) {
        log("vfs_mp_path_alloc: Failed to allocate memory for mountpoint\n", RED);
        return NULL;
    }
    mp->filesystem = NULL;
    mp->mount_point = NULL;
    mp->device_name = NULL;
    mp->next_mountpoint = NULL;
    mp->data_pointer = NULL;

    refcount_init(&mp->refcount);
    return mp;
}

static int vfs_assign_mp_fs(struct vfs_mountpoint* mp, int type) {
    mp->filesystem = vfs_find_fs(type);
    if (!mp->filesystem) {
        log_uint("vfs_assign_mp_fs: Failed to find filesystem type ", type);
        return -1;
    }
    return 0;
}

static int vfs_mp_path_alloc(struct vfs_mountpoint* mp, char* mount_point) {
    mp->mount_point = (char*)kmalloc(flopstrlen(mount_point) + 1);
    if (!mp->mount_point) {
        log("vfs_mp_path_alloc: Failed to allocate memory for mountpoint\n", RED);
        return -1;
    }
    flopstrcopy(mp->mount_point, mount_point, flopstrlen(mount_point) + 1);
    return 0;
}

static int vfs_mp_dev_alloc(struct vfs_mountpoint* mp, char* device) {
    mp->device_name = (char*)kmalloc(flopstrlen(device) + 1);
    if (!mp->device_name) {
        log("vfs_mp_dev_alloc: Failed to allocate memory for device name\n", RED);
        return -1;
    }
    flopstrcopy(mp->device_name, device, flopstrlen(device) + 1);
    return 0;
}

static struct vfs_mountpoint* vfs_create_mountpoint(char* device, char* mount_point, int type) {
    struct vfs_mountpoint* mp = vfs_mp_struc_alloc();
    if (!mp)
        return NULL;

    if (vfs_assign_mp_fs(mp, type) != 0) {
        kfree(mp, sizeof(struct vfs_mountpoint));
        return NULL;
    }

    if (vfs_mp_path_alloc(mp, mount_point) != 0) {
        kfree(mp, sizeof(struct vfs_mountpoint));
        return NULL;
    }

    if (vfs_mp_dev_alloc(mp, device) != 0) {
        kfree(mp->mount_point, flopstrlen(mp->mount_point) + 1);
        kfree(mp, sizeof(struct vfs_mountpoint));
        return NULL;
    }

    return mp;
}

static void vfs_add_mountpoint(struct vfs_mountpoint* mp) {
    spinlock(&mp_list.lock);
    if (mp_list.tail == NULL) {
        mp_list.tail = mp;
        mp_list.head = mp;
    } else {
        mp_list.tail->next_mountpoint = mp;
        mp_list.tail = mp;
    }
    refcount_inc_not_zero(&mp->refcount);
    spinlock_unlock(&mp_list.lock, true);
}

static void vfs_free_mountpoint(struct vfs_mountpoint* mp) {
    if (!mp) {
        log("vfs_free_mountpoint: mp is NULL\n", RED);
        return;
    }
    if (mp->mount_point)
        kfree(mp->mount_point, flopstrlen(mp->mount_point) + 1);
    if (mp->device_name)
        kfree(mp->device_name, flopstrlen(mp->device_name) + 1);
    kfree(mp, sizeof(struct vfs_mountpoint));
}

static void vfs_remove_mountpoint(struct vfs_mountpoint* mp) {
    struct vfs_mountpoint *m, *prev = NULL;

    spinlock(&mp_list.lock);

    for (m = mp_list.head; m != NULL; prev = m, m = m->next_mountpoint) {
        if (m == mp) {
            if (prev)
                prev->next_mountpoint = mp->next_mountpoint;
            else
                mp_list.head = mp->next_mountpoint;
            if (mp == mp_list.tail)
                mp_list.tail = prev;
            break;
        }
    }
    if (refcount_dec_and_test(&mp->refcount)) {
        spinlock_unlock(&mp_list.lock, true);
        vfs_free_mountpoint(mp);
        return;
    }

    spinlock_unlock(&mp_list.lock, true);
}




static int vfs_node_alloc(struct vfs_node **node, struct vfs_mountpoint *mp, int mode) {
    *node = (struct vfs_node *)kmalloc(sizeof(struct vfs_node));
    if (!*node) {
        log("vfs_node_alloc: Failed to allocate node\n", RED);
        return -1;
    }
    (*node)->mountpoint = mp;
    (*node)->vfs_mode = mode;
    refcount_init(&(*node)->refcount);

    refcount_inc_not_zero(&mp->refcount);

    return 0;
}

static int vfs_create_file_if_needed(struct vfs_mountpoint *mp, char *path, int mode) {
    if ((mode & VFS_MODE_CREATE) != VFS_MODE_CREATE)
        return 0;
    if (mp->filesystem->op_table.create == NULL) {
        log("vfs_create_file_if_needed: Filesystem does not support create\n", RED);
        return -1;
    }
    return mp->filesystem->op_table.create(mp, path);
}

int vfs_seek(struct vfs_node *node, unsigned long offset, unsigned char whence);
static int vfs_seek_if_append(struct vfs_node *n) {
    if ((n->vfs_mode & VFS_MODE_APPEND) == VFS_MODE_APPEND) {
        return vfs_seek(n, 0, VFS_SEEK_END);
    }
    return 0;
}

static struct vfs_mountpoint* vfs_resolve_mountpoint_and_path(char* name, char **relative_path) {
    size_t len = flopstrlen(name) + 1;
    char *filename = kmalloc(len);
    if (!filename) return NULL;

    flopstrcopy(filename, name, len);
    struct vfs_mountpoint *mp = vfs_file_to_mountpoint(filename);
    if (!mp) {
        kfree(filename, sizeof(filename));
        return NULL;
    }

    *relative_path = filename + flopstrlen(mp->mount_point);
    return mp;
}


static int vfs_try_open(struct vfs_node *h, struct vfs_mountpoint *mp, char *path) {
    if (mp->filesystem->op_table.open == NULL) {
        log("vfs_try_open: Filesystem type does not support opening\n", RED);
        return -1;
    }
    return mp->filesystem->op_table.open(h, path) != NULL ? 0 : -1;
}



static struct vfs_mountpoint* vfs_get_mountpoint_for_create(char* name, char **relative_path) {
    char filename[256];
    flopstrcopy(filename, name, flopstrlen(name) + 1);
    struct vfs_mountpoint *mp = vfs_file_to_mountpoint(filename);
    if (!mp) {
        log("vfs_create: Mountpoint not found\n", RED);
        return NULL;
    }
    *relative_path = filename + flopstrlen(mp->mount_point);
    refcount_inc_not_zero(&mp->refcount);
    return mp;
}

static int vfs_internal_create(struct vfs_mountpoint *mp, char *relative_path) {
    if (mp->filesystem->op_table.create != NULL) {
        return mp->filesystem->op_table.create(mp, relative_path);
    } else {
        log("vfs_create: Filesystem type does not support creating files\n", RED);
        return -1;
    }
}



int vfs_free_node(struct vfs_node* node) {
    int errcode = 0;
    if (node->mountpoint->filesystem->op_table.close != NULL) {
        errcode = node->mountpoint->filesystem->op_table.close(node);
    }

    if (refcount_dec_and_test(&node->mountpoint->refcount)) {
        vfs_free_mountpoint(node->mountpoint);
    }

    kfree(node, sizeof(struct vfs_node));
    return errcode ? errcode : -1;
}


static int vfs_check_write_permissions(struct vfs_node* node) {
    if (node == NULL) {
        log("vfs_write: node is NULL\n", RED);
        return -1;
    }
    if ((node->vfs_mode & VFS_MODE_W) != VFS_MODE_W) {
        log("vfs_write: node is not opened for writing\n", RED);
        return -1;
    }
    return 0;
}

static int vfs_internal_write(struct vfs_node* node, unsigned char* buffer, unsigned long size) {
    int ret = -1;
    if (node->mountpoint->filesystem->op_table.write != NULL) {
        refcount_inc_not_zero(&node->mountpoint->refcount);
        ret = node->mountpoint->filesystem->op_table.write(node, buffer, size);
        if (refcount_dec_and_test(&node->mountpoint->refcount)) {
            vfs_free_mountpoint(node->mountpoint);
        }
        return ret;
    }
    log("vfs_write: Filesystem type does not support writing\n", RED);
    return -1;
}

static void vfs_seek_if_append_after_write(struct vfs_node* node) {
    if ((node->vfs_mode & VFS_MODE_APPEND) == VFS_MODE_APPEND) {
        vfs_seek(node, 0, VFS_SEEK_END);
    }
}


struct vfs_directory_list* vfs_directory_list_create(void) {
    struct vfs_directory_list* list = (struct vfs_directory_list*)kmalloc(sizeof(struct vfs_directory_list));
    if (!list)
        return NULL;
    list->head = NULL;
    list->tail = NULL;
    return list;
}

void vfs_directory_list_add(struct vfs_directory_list* list, const char* name, int type) {
    struct vfs_directory_entry* entry = (struct vfs_directory_entry*)kmalloc(sizeof(struct vfs_directory_entry));
    if (!entry)
        return;
    flopstrcopy(entry->name, name, flopstrlen(name) + 1);
    entry->type = type;
    entry->next = NULL;
    if (!list->head) {
        list->head = entry;
        list->tail = entry;
    } else {
        list->tail->next = entry;
        list->tail = entry;
    }
}

void vfs_directory_list_free(struct vfs_directory_list* list) {
    struct vfs_directory_entry* entry = list->head;
    while (entry) {
        struct vfs_directory_entry* next = entry->next;
        kfree(entry, sizeof(struct vfs_directory_entry));
        entry = next;
    }
    kfree(list, sizeof(struct vfs_directory_list));
}

// op table functions
int vfs_mount(char* device, char *mount_point, int type) {
    struct vfs_mountpoint* mp = vfs_create_mountpoint(device, mount_point, type);
    if (!mp)
        return -1;

    vfs_add_mountpoint(mp);

    if (mp->filesystem->op_table.mount == NULL) {
        log_uint("vfs_mount: fs type doesnt support mounting, type: ", type);
        vfs_remove_mountpoint(mp);
        return -1;  
    }
    mp->data_pointer = mp->filesystem->op_table.mount(device, mount_point, type);
    if (!mp->data_pointer) {
        log("vfs_mount: Failed to mount filesystem\n", RED);
        vfs_remove_mountpoint(mp);
        return -1;
    }
    return 0;
}

int vfs_unmount(char *mount_point) {
    struct vfs_mountpoint *mp;
    char filename[256], *nptr;
    nptr = (char *)&filename;
    flopstrcopy(nptr, mount_point, flopstrlen(mount_point) + 1);

    for (mp = mp_list.tail; mp != NULL; mp = mp->next_mountpoint) {
        if (flopstrcmp(mp->mount_point, nptr) == 0) {
            break;
        }
    }

    if (mp == NULL) {
        log("vfs_unmount: Mountpoint not found\n", RED);
        return -1;
    }

    if (mp->filesystem->op_table.unmount == NULL) {
        log("vfs_unmount: Filesystem type does not support unmounting ?? \n", RED);
        return -1;
    }

    mp->filesystem->op_table.unmount(mp, nptr);
    vfs_remove_mountpoint(mp);

    return 0;
}

struct vfs_directory_list* vfs_listdir(struct vfs_mountpoint *mp, char *path) {
    if (mp == NULL || path == NULL) {
        log("vfs_listdir: Mountpoint or path is NULL\n", RED);
        return NULL;
    }
    if (mp->filesystem->op_table.listdir == NULL) {
        log("vfs_listdir: Filesystem type does not support listing directories\n", RED);
        return NULL;
    }
    return mp->filesystem->op_table.listdir(mp, path);
}

int vfs_close(struct vfs_node* node) {
    if (node == NULL) {
        log("vfs_close: node is NULL\n", RED);
        return -1;
    }
    vfs_free_node(node);
    return 0;
}

int vfs_read(struct vfs_node* node, unsigned char* buffer, unsigned long size) {
    if (node == NULL) {
        log("vfs_read: node is NULL\n", RED);
        return -1;
    }
    if ((node->vfs_mode & VFS_MODE_R) != VFS_MODE_R) {
        log("vfs_read: node is not opened for reading\n", RED);
        return -1;
    }
    int ret = -1;
    if (node->mountpoint->filesystem->op_table.read != NULL) {
        // refcount starts at 1, so only inc if sharing
        ret = node->mountpoint->filesystem->op_table.read(node, buffer, size);
        return ret;
    }
    log("vfs_read: Filesystem type does not support reading\n", RED);
    return -1;
}

int vfs_create(char* name) {
    struct vfs_mountpoint *mp;
    char *relative_path = NULL;

    mp = vfs_get_mountpoint_for_create(name, &relative_path);
    if (!mp)
        return -1;

    int ret = vfs_internal_create(mp, relative_path);

    if (refcount_dec_and_test(&mp->refcount)) {
        vfs_free_mountpoint(mp);
    }

    return ret;
}

int vfs_write(struct vfs_node* node, unsigned char* buffer, unsigned long size) {
    int errcode;

    if (vfs_check_write_permissions(node) != 0)
        return -1;

    errcode = vfs_internal_write(node, buffer, size);
    if (errcode != -1) {
        vfs_seek_if_append_after_write(node);
        return errcode;
    }

    return -1;
}

struct vfs_node* vfs_open(char* name, int mode) {
    struct vfs_node *n = NULL;
    char *relative_path = NULL;
    struct vfs_mountpoint *mp = vfs_resolve_mountpoint_and_path(name, &relative_path);

    if (!mp) {
        log("vfs_open: Mountpoint not found\n", RED);
        return NULL;
    }

    if (vfs_node_alloc(&n, mp, mode) != 0)
        return NULL;

    if (vfs_try_open(n, mp, relative_path) == 0) {
        vfs_seek_if_append(n);
        return n;
    }

    if (vfs_create_file_if_needed(mp, relative_path, mode) == 0) {
        if (vfs_try_open(n, mp, relative_path) == 0) {
            vfs_seek_if_append(n);
            return n;
        }
    }

    if (refcount_dec_and_test(&mp->refcount)) {
        vfs_free_mountpoint(mp);
    }
    kfree(n, sizeof(struct vfs_node));
    return NULL;
}

int vfs_seek(struct vfs_node* node, unsigned long offset, unsigned char whence) {
    if (node == NULL) {
        log("vfs_seek: node is NULL\n", RED);
        return -1;
    }
    if (node->mountpoint->filesystem->op_table.seek != NULL) {
        return node->mountpoint->filesystem->op_table.seek(node, offset, whence);
    }

    log("vfs_seek: Filesystem type does not support seeking\n", RED);
    return -1;
}


int vfs_ctrl(struct vfs_node* node, unsigned long command, unsigned long arg) {
    if (node == NULL) {
        log("vfs_ctrl: node is NULL\n", RED);
        return -1;
    }
    if (node->mountpoint->filesystem->op_table.ctrl != NULL) {
        return node->mountpoint->filesystem->op_table.ctrl(node, command, arg);
    }
    return -1;
}