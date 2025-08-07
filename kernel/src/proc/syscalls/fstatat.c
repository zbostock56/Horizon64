/**
 * @file fstatat.c
 * @author Zack Bostock
 * @brief Functionality pertaining to fstatat system call
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <proc/syscalls/fstatat.h>
#include <proc/syscall.h>
#include <proc/ctxsw.h>

#include <fs/vfs.h>

#include <sys/smp.h>

#include <string.h>
#include <common/kprint.h>

/**
 * @brief Get file status
 *
 * @param dirfh Directory file handle
 * @param path Path to file to get status of
 * @param statbuf Buffer to copy into
 * @param flags How to read the file
 * @return int64_t 0 if success, -1 if failure
 */
int64_t sys_fstatat(int64_t dirfh, const char *path, int64_t statbuf,
                    int64_t flags) {
    (void) flags;
    cpu_set_errno(0);
    char full_path[VFS_MAX_PATH_LEN] = {0};
    if (sys_get_full_path(dirfh, path, full_path) == SYSCALL_FAIL) {
        return -1;
    }

    VFS_TNODE *tnode = vfs_path_to_node(full_path, NO_CREATE, 0);

    if (tnode && tnode->stat.nlink > 0) {
        VFS_STAT *stat = (VFS_STAT *) statbuf;
        memcpy(stat, &(tnode->stat), sizeof(VFS_STAT));
        klogd("sys_fstatat: successful file status retrieval for \"%s\"\n", full_path);
        return 0;
    } else {
        klogd("sys_fstatat: Failed to retrieve file status for \"%s\"\n", full_path);
        cpu_set_errno(ENOENT);
        return -1;
    }
}
