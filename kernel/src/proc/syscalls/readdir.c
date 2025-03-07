/**
 * @file readdir.c
 * @author Zack Bostock
 * @brief Functionality pertaining to readdir system call
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <proc/syscalls/readdir.h>
#include <proc/syscall.h>
#include <proc/process.h>

#include <fs/vfs.h>

#include <sys/smp.h>

#include <common/kprint.h>
#include <common/vector.h>

/**
 * @brief Read a directory
 *
 * @param handle Handle of directory to read
 * @param buff Directory entry buffer
 * @return int64_t 0 if success, -1 if failure
 */
int64_t sys_readdir(int64_t handle, uint64_t buff) {
    DIRENT *de = (DIRENT *) buff;
    VFS_NODE_DESC *fd = vfs_handle_to_fd(handle);

    cpu_set_errno(0);

    if (!fd) {
        cpu_set_errno(EINVAL);
        return -1;
    }

    /* Check to make sure the type is either a directory or mount point */
    if (!(fd->inode->type == VFS_DIRECTORY || fd->inode->type == VFS_MOUNT_POINT)) {
        cpu_set_errno(ENOTDIR);
        return -1;
    }

    if (!fd->current_dir_ent) {
        if (vector_len(&fd->inode->child) == 0) {
            /* End of directory */
            return -1;
        }
        fd->current_dir_ent = vector_at(&fd->inode->child, 0);
        fd->current_dir_index = 0;
    } else {
        if (fd->current_dir_index >= vector_len(&fd->inode->child) - 1) {
            /* End of directory */
            fd->current_dir_ent = NULL;
            return -1;
        }
        fd->current_dir_ent = vector_at(&fd->inode->child, fd->current_dir_index + 1);
        fd->current_dir_index++;
    }

    strcpy(de->name, fd->current_dir_ent->name);
    de->ino = fd->current_dir_ent->stat.ino;
    de->off = 0;
    de->rec_len = sizeof(DIRENT);
    de->type = DT_UNKNOWN;
    return 0;
}
