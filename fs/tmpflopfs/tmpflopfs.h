#ifndef TMPFS_H
#define TMPFS_H

#include <stdint.h>

#define TMPFS_MAX_NODES 256
#define TMPFS_NAME_MAX 64
#define TMP_MAX_NAME_LEN      64
#define TMP_MAX_CHILDREN      64
#define TMP_MAX_PATH_LENGTH   256
#define TMP_MAX_NODES         256
#define TMP_MAX_FILE_SIZE     4096
#define TMP_DISK_SIZE         (10 * 1024 * 1024)


#define TMPFS_ADDR_SPACE_SIZE 0x100000 
typedef enum {
    TMP_NODE_FILE,
    TMP_NODE_DIR
} TmpNodeType;



#define TMP_PERM_READ   0x4
#define TMP_PERM_WRITE  0x2
#define TMP_PERM_EXEC   0x1
// Permissions: 9 bits, rwxrwxrwx (owner/group/other)
typedef uint16_t TmpPerms;
typedef struct {
    TmpPerms perms;
} TmpNodePerms;




typedef struct TmpNode {
    TmpNodeType type;
    char name[TMP_MAX_NAME_LEN];
    uint32_t size; 
    uint32_t parent_idx; 
    uint32_t children[TMP_MAX_CHILDREN]; // Indexes of children
    uint32_t child_count;
    union {
        uint8_t file_data[TMP_MAX_FILE_SIZE]; 
        struct {} _dir;
    };
} TmpNode;

typedef struct TmpFileSystem {
    TmpNode nodes[TMP_MAX_NODES];
    uint32_t node_count;
    uint32_t root_idx;
} TmpFileSystem;

void tmpfs_init();
TmpFileSystem *tmpfs_get_instance(void) ;
int tmpfs_mkdir(TmpFileSystem *fs, const char *path, const char *name);
int tmpfs_create_file(TmpFileSystem *fs, const char *path, const char *name, const void *data, uint32_t size);
int tmpfs_delete_node(TmpFileSystem *fs, const char *path, int recursive) ;
int tmpfs_write_file(TmpFileSystem *fs, const char *path, const void *data, uint32_t size, uint32_t offset);
#endif // TMPFS_H
