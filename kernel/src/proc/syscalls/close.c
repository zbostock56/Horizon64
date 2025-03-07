/**
 * @file close.c
 * @author Zack Bostock
 * @brief Functionality pertaining to close system call
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <proc/syscalls/close.h>
#include <proc/syscall.h>
#include <proc/process.h>
#include <proc/ctxsw.h>

#include <fs/vfs.h>

#include <sys/cpu.h>
#include <sys/smp.h>

#include <common/lock.h>
#include <common/kprint.h>

extern LOCK vfs_lock;

/**
 * @brief System call implementation for closing a file
 *
 * @param fh File handle to close
 * @return int64_t -1 if failure, 0 upon success
 */
int64_t sys_close(int64_t fh) {
    if (fh == VFS_INVALID_HANDLE) {
        cpu_set_errno(EINVAL);
        return -1;
    }

    PROCESS *pcurr = sched_get_curr_proc();

    klogd("sys_close: closing file handle %d\n", fh);

    if (pcurr) {
        LOCK_LOCK(&vfs_lock);

        /* Check whether there is a file redirection */
        for (size_t i = 0; i < vector_len(&pcurr->dup_list); i++) {
            FILE_DUP dup = vector_at(&pcurr->dup_list, i);
            if (dup.new == fh) {
                /* Close original file and delete from dup list */
                if (dup.prev != STDIN && dup.prev != STDOUT && dup.prev != STDERR) {
                    klogd("sys_close: close dup file handle %d\n", dup.prev);

                    /* Without unlocking first, this will cause a dead lock */
                    UNLOCK_LOCK(&vfs_lock);
                    vfs_close(dup.prev);
                    LOCK_LOCK(&vfs_lock);
                }
                vector_erase(&pcurr->dup_list, i);
                break;
            }
            if (dup.prev == fh) {
                UNLOCK_LOCK(&vfs_lock);

                /* Do not close if mapping to another file handle */
                klogd("sys_close: do not close dup file handle %d -> %d\n",
                      dup.new, fh);
                cpu_set_errno(EINVAL);
                return -1;
            }
        }
        UNLOCK_LOCK(&vfs_lock);
    }

    if (fh == STDIN || fh == STDOUT || fh == STDERR) {
        return 0;
    }

    return vfs_close(fh);
}