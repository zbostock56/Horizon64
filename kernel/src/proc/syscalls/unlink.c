/**
 * @file unlink.c
 * @author Zack Bostock
 * @brief Functionality pertaining to unlink system call
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <proc/syscalls/unlink.h>
#include <proc/syscall.h>
#include <proc/process.h>
#include <proc/ctxsw.h>

#include <sys/smp.h>

#include <common/kprint.h>

/**
 * @brief Unlinks a file
 *
 * @param path Path to unlink
 * @return int64_t -1 if failure, 0 if success
 */
int64_t sys_unlink(char *path) {
    cpu_set_errno(0);

    klogd("sys_unlink: unlinking %s\n", path);

    char full_path[VFS_MAX_PATH_LEN] = {0};
    if (sys_get_full_path(VFS_FW_CWD, path, full_path) == SYSCALL_FAIL) {
        cpu_set_errno(EINVAL);
        return -1;
    } else {
        /* Check if the directory exists */
        size_t len = strlen(full_path);
        if (len == 0) {
            cpu_set_errno(EINVAL);
            return -1;
        }


        /* Replace trailing slash with nul-byte */
        for (int64_t i = len - 1; i >= 0; i--) {
            if (full_path[i] == '/') {
                full_path[i] = '\0';
                break;
            }
        }

        if (strlen(full_path) > 0) {
            VFS_TNODE *tnode = vfs_path_to_node(full_path, NO_CREATE, 0);
            if (!tnode) {
                kloge("sys_unlink: directory \"%s\" doesn't exist\n", full_path);
                cpu_set_errno(ENOENT);
                return -1;
            }
        }

        /* TODO: Evaluate if this is needed */
        if (sys_get_full_path(VFS_FW_CWD, path, full_path) == SYSCALL_FAIL) {
            cpu_set_errno(EINVAL);
            return -1;
        }
    }

    VFS_TNODE *tnode = vfs_path_to_node(full_path, NO_CREATE, 0);
    if (!tnode) {
        cpu_set_errno(ENOENT);
        return -1;
    }

    VFS_INODE *parent_inode = tnode->parent;
    for (size_t i = 0; i < vector_len(&(parent_inode->child)); i++) {
        if (vector_at(&(parent_inode->child), i) == tnode) {
            if (tnode->inode->references == 0) {
                vector_erase(&(parent_inode->child), i);
                return 0;
            } else {
                klogw("sys_unlink: fialed because reference count of \"%s\" is %d\n",
                      path, tnode->inode->references);
                return -1;
            }
        }
    }

    cpu_set_errno(ENOENT);
    return -1;
}
