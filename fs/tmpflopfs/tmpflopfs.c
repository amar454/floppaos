#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "../../drivers/vga/vgahandler.h"
#include "../../drivers/time/floptime.h"
#include "../../apps/echo.h"
#include "../../lib/str.h"
#include "../../lib/logging.h"
#include "../../mem/vmm.h"
#include "tmpflopfs.h"
#include "../../mem/utils.h"

static TmpFileSystem *tmpfs_instance = NULL;

void tmpfs_init(void) {
    if (tmpfs_instance) return;

    page_attrs_t attrs = {
        .present = 1,
        .rw = 1,
        .user = 0
    };

    tmpfs_instance = (TmpFileSystem*)vmm_create_address_space(TMPFS_ADDR_SPACE_SIZE, attrs);
    if (!tmpfs_instance) {
        log("tmpfs_init: failed to allocate virtual memory for FS", RED);
        return;
    }

    flop_memset(tmpfs_instance, 0, sizeof(TmpFileSystem));
    tmpfs_instance->node_count = 1;
    tmpfs_instance->root_idx = 0;

    TmpNode *root = &tmpfs_instance->nodes[0];
    root->type = TMP_NODE_DIR;
    flopstrcopy(root->name, "/", 1);
    root->parent_idx = UINT32_MAX;
}

TmpFileSystem *tmpfs_get_instance(void) {
    return tmpfs_instance;
}

void tmpfs_destroy(TmpFileSystem* fs) {
    if (!fs) return;

    vmm_free_address_space((void*)fs, TMPFS_ADDR_SPACE_SIZE);
}

TmpNode *tmpfs_find_node(TmpFileSystem *fs, const char *path) {
    if (!fs || !path || path[0] != '/') return NULL;
    if (flopstrcmp(path, "/") == 0) return &fs->nodes[fs->root_idx];

    char temp[TMP_MAX_PATH_LENGTH];
    flopstrcopy(temp, path, TMP_MAX_PATH_LENGTH - 1);
    temp[TMP_MAX_PATH_LENGTH - 1] = '\0';

    TmpNode *curr = &fs->nodes[fs->root_idx];
    char *token = flopstrtok(temp, "/");
    while (token && curr) {
        bool found = false;
        for (uint32_t i = 0; i < curr->child_count; ++i) {
            TmpNode *child = &fs->nodes[curr->children[i]];
            if (flopstrcmp(child->name, token) == 0) {
                curr = child;
                found = true;
                break;
            }
        }
        if (!found) return NULL;
        token = flopstrtok(NULL, "/");
    }
    return curr;
}

int tmpfs_mkdir(TmpFileSystem *fs, const char *path, const char *name) {
    if (!fs || fs->node_count >= TMP_MAX_NODES) return -1;
    TmpNode *parent = tmpfs_find_node(fs, path);
    if (!parent || parent->type != TMP_NODE_DIR || parent->child_count >= TMP_MAX_CHILDREN)
        return -1;
    for (uint32_t i = 0; i < parent->child_count; ++i)
        if (flopstrcmp(fs->nodes[parent->children[i]].name, name) == 0)
            return -1;
    TmpNode *dir = &fs->nodes[fs->node_count];
    flop_memset(dir, 0, sizeof(TmpNode));
    dir->type = TMP_NODE_DIR;
    flopstrcopy(dir->name, name, TMP_MAX_NAME_LEN - 1);
    dir->parent_idx = (uint32_t)(parent - fs->nodes);
    parent->children[parent->child_count++] = fs->node_count;
    fs->node_count++;
    return 0;
}

int tmpfs_create_file(TmpFileSystem *fs, const char *path, const char *name, const void *data, uint32_t size) {
    if (!fs || fs->node_count >= TMP_MAX_NODES || size > TMP_MAX_FILE_SIZE) return -1;
    TmpNode *parent = tmpfs_find_node(fs, path);
    if (!parent || parent->type != TMP_NODE_DIR || parent->child_count >= TMP_MAX_CHILDREN)
        return -1;
    for (uint32_t i = 0; i < parent->child_count; ++i)
        if (flopstrcmp(fs->nodes[parent->children[i]].name, name) == 0)
            return -1;
    TmpNode *file = &fs->nodes[fs->node_count];
    flop_memset(file, 0, sizeof(TmpNode));
    file->type = TMP_NODE_FILE;
    flopstrcopy(file->name, name, TMP_MAX_NAME_LEN - 1);
    file->parent_idx = (uint32_t)(parent - fs->nodes);
    file->size = size;
    flop_memcpy(file->file_data, data, size);
    parent->children[parent->child_count++] = fs->node_count;
    fs->node_count++;
    return 0;
}
/**
 * @param recursive if set, delete all dirs/files in directory.
 */
int tmpfs_delete_node(TmpFileSystem *fs, const char *path, int recursive) {
    if (!fs || !path) return -1;
    TmpNode *node = tmpfs_find_node(fs, path);
    if (!node || node == &fs->nodes[fs->root_idx]) return -1; // Don't delete root

    TmpNode *parent = &fs->nodes[node->parent_idx];
    // Remove from parent's children
    bool found = false;
    for (uint32_t i = 0; i < parent->child_count; ++i) {
        if (parent->children[i] == (uint32_t)(node - fs->nodes)) {
            found = true;
            for (uint32_t j = i; j < parent->child_count - 1; ++j)
                parent->children[j] = parent->children[j + 1];
            parent->child_count--;
            break;
        }
    }
    if (!found) return -1;


    if (node->type == TMP_NODE_DIR) {
        if (node->child_count > 0 && !recursive) {

            parent->children[parent->child_count++] = (uint32_t)(node - fs->nodes);
            return -1;
        }
        while (node->child_count > 0 && recursive) {
            uint32_t child_idx = node->children[0];
            char child_path[TMP_MAX_PATH_LENGTH];
            char parent_path[TMP_MAX_PATH_LENGTH] = "";
            TmpNode *tmp = node;
            while (tmp->parent_idx != UINT32_MAX) {
                char temp[TMP_MAX_PATH_LENGTH];
                flopsnprintf(temp, sizeof(temp), "/%s%s", tmp->name, parent_path);
                flopstrcopy(parent_path, temp, sizeof(parent_path));
                tmp = &fs->nodes[tmp->parent_idx];
            }
            flopsnprintf(child_path, sizeof(child_path), "%s/%s", parent_path[0] ? parent_path : "/", fs->nodes[child_idx].name);
            tmpfs_delete_node(fs, child_path, 1);
        }
        if (node->child_count > 0 && !recursive) {
            // Should not reach here, but just in case
            return -1;
        }
    }

    uint32_t idx = (uint32_t)(node - fs->nodes);
    if (idx != fs->node_count - 1) {
        fs->nodes[idx] = fs->nodes[fs->node_count - 1];
        if (fs->nodes[idx].parent_idx != UINT32_MAX) {
            TmpNode *moved_parent = &fs->nodes[fs->nodes[idx].parent_idx];
            for (uint32_t i = 0; i < moved_parent->child_count; ++i)
                if (moved_parent->children[i] == fs->node_count - 1)
                    moved_parent->children[i] = idx;
        }
        if (fs->nodes[idx].type == TMP_NODE_DIR) {
            for (uint32_t i = 0; i < fs->nodes[idx].child_count; ++i) {
                TmpNode *child = &fs->nodes[fs->nodes[idx].children[i]];
                child->parent_idx = idx;
            }
        }
    }
    fs->node_count--;
    return 0;
}

// Write to a file (overwrite from offset)
int tmpfs_write_file(TmpFileSystem *fs, const char *path, const void *data, uint32_t size, uint32_t offset) {
    if (!fs || !data) return -1;
    TmpNode *file = tmpfs_find_node(fs, path);
    if (!file || file->type != TMP_NODE_FILE) return -1;
    if (offset > TMP_MAX_FILE_SIZE || size > TMP_MAX_FILE_SIZE || offset + size > TMP_MAX_FILE_SIZE)
        return -1;
    flop_memcpy(file->file_data + offset, data, size);
    if (offset + size > file->size)
        file->size = offset + size;
    return 0;
}


static TmpNodePerms node_perms[TMP_MAX_NODES];

int tmpfs_set_permissions(TmpFileSystem *fs, const char *path, TmpPerms perms) {
    if (!fs || !path) return -1;
    TmpNode *node = tmpfs_find_node(fs, path);
    if (!node) return -1;
    uint32_t idx = (uint32_t)(node - fs->nodes);
    node_perms[idx].perms = perms;
    return 0;
}

TmpPerms tmpfs_get_permissions(TmpFileSystem *fs, const char *path) {
    if (!fs || !path) return 0;
    TmpNode *node = tmpfs_find_node(fs, path);
    if (!node) return 0;
    uint32_t idx = (uint32_t)(node - fs->nodes);
    return node_perms[idx].perms;
}

void tmpfs_list_dir(TmpFileSystem *fs, TmpNode *dir) {
    if (!dir || dir->type != TMP_NODE_DIR) return;
    for (uint32_t i = 0; i < dir->child_count; ++i) {
        TmpNode *child = &fs->nodes[dir->children[i]];
        char buffer[TMP_MAX_NAME_LEN + 32];
        if (child->type == TMP_NODE_DIR) {
            flopsnprintf(buffer, sizeof(buffer), "[DIR] %s\n", child->name);
        } else {
            flopsnprintf(buffer, sizeof(buffer), "[FILE] %s (%u bytes)\n", child->name, child->size);
        }
        echo(buffer, WHITE);
    }
}
