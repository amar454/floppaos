#include "../vfs/vfs.h"
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

struct procfs {
    uint32_t procfs_count;
    struct vfs_op_tbl procfs_ops;
    struct vfs_fs* procfs_fs;
    spinlock_t procfs_lock;
    struct vfs_directory_entry* procfs_dir_entries;
};
