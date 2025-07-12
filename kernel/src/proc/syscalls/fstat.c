/**
 * @file fstat.c
 * @author Zack Bostock
 * @brief Functionality pertaining to fstat system call
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <proc/syscalls/fstat.h>
#include <proc/syscall.h>
#include <proc/ctxsw.h>

#include <sys/smp.h>

#include <fs/vfs.h>

#include <common/kprint.h>
#include <common/memory.h>

/**
 * @brief Gets file status
 *
 * @param handle Handle of file to get status of
 * @param statbuf Buffer to copy result into
 * @return int64_t 0 if success, -1 if failure
 */
int64_t sys_fstat(int64_t handle, int64_t statbuf) {
    cpu_set_errno(0);
    VFS_STAT *stat = (VFS_STAT *)statbuf;
    if (handle == STDIN || handle == STDOUT || handle == STDERR) {
        /* Even though this makes no sense to stat these, set to zero */
        /* for compatibility in case user accidentially uses the result */
        memset(stat, 0, sizeof(VFS_STAT));
        klogd("sys_fstat: success with handle %d\n", handle);
        return 0;
    }

    VFS_NODE_DESC *fd = vfs_handle_to_fd(handle);
     if (fd) {
        memcpy(stat, &(fd->tnode->stat), sizeof(VFS_STAT));
        klogd("sys_fstat: success with handle %d\n", handle);
        return 0;
     } else {
        kloge("sys_fstat: failed with handle %d\n", handle);
        cpu_set_errno(EINVAL);
        return -1;
     }
}