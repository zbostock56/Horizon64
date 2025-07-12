/**
 * @file readlink.c
 * @author Zack Bostock
 * @brief Functionality pertaining to readlink system call
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <proc/syscalls/readlink.h>
#include <proc/syscall.h>

#include <fs/vfs.h>

#include <sys/smp.h>

#include <common/string.h>
#include <common/kprint.h>

/**
 * @brief Reads a symbolic link
 *
 * @param dirfh Directory handle
 * @param path Path to symbolic link
 * @param buffer Buffer to copy into
 * @param max_size Size of buffer to copy into
 * @return int64_t -1 if failure, or strlen of symlink on success
 */
int64_t sys_readlink(int64_t dirfh, const char *path, void *buffer,
                     size_t max_size) {
    cpu_set_errno(0);

    char full_path[VFS_MAX_PATH_LEN] = {0};
    sys_get_full_path(dirfh, path, full_path);

    VFS_TNODE *tnode = vfs_path_to_node(full_path, NO_CREATE, 0);
    if (!tnode || tnode->inode->type != VFS_SYMLINK) {
        cpu_set_errno(EINVAL);
        return -1;
    }

    if ((size_t)strlen(tnode->inode->symlink) < max_size) {
        klogd("sys_readlink: %s -> %s\n", full_path, tnode->inode->symlink);
        strcpy(buffer, tnode->inode->symlink);
    } else {
        cpu_set_errno(EINVAL);
        return -1;
    }

    return strlen(buffer);
}