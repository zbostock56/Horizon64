/**
 * @file write.c
 * @author Zack Bostock
 * @brief Functionality pertaining to write system call
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <proc/syscalls/write.h>
#include <proc/syscall.h>
#include <proc/process.h>
#include <proc/ctxsw.h>

#include <fs/vfs.h>

#include <sys/smp.h>
#include <sys/cpu.h>

#include <common/kprint.h>
#include <common/lock.h>

extern LOCK vfs_lock;

/**
 * @brief System call implementation for writing to a file
 *
 * @param fh File handle to write to
 * @param buff Buffer of which to write
 * @param count Number of bytes to write
 * @return int64_t Bytes written, or -1 if failure
 */
int64_t sys_write(int64_t fh, void *buff, size_t count) {
    if (!buff || count == 0 || fh == VFS_INVALID_HANDLE) {
        cpu_set_errno(EINVAL);
        return -1;
    }

    PROCESS *pcurr = sched_get_curr_proc();

    cpu_set_errno(0);

    if (fh == STDOUT || fh == STDERR) {
        uint8_t found = FALSE;
        VFS_HANDLE oldfh = -1;
        if (pcurr) {
            LOCK_LOCK(&vfs_lock);
            /* Check whether it is redirected from some file */
            for (size_t i = 0; i < vector_len(&pcurr->dup_list); i++) {
                FILE_DUP dup = vector_at(&pcurr->dup_list, i);
                if (dup.new == fh) {
                    oldfh = dup.prev;
                    found = TRUE;
                    break;
                } else if (dup.prev == fh) {
                    oldfh = dup.new;
                    found = TRUE;
                    break;
                }
            }
            UNLOCK_LOCK(&vfs_lock);
        }

        if (found) {
            klogd("sys_write: writing %d bytes to fh %d -> oldfh %d\n",
                  count, fh, oldfh);
            return vfs_write(oldfh, buff, count);
        } else {
            VFS_HANDLE ttyfh = vfs_open("/dev/tty", VFS_READ_WRITE);
            if (ttyfh != VFS_INVALID_HANDLE) {
                int64_t len = vfs_write(ttyfh, buff, count);
                vfs_close(ttyfh);
                return len;
            } else {
                kloge("ttyfs file handle is invalid!\n");
                halt();
            }
            return 0;
        }
    }

    if (fh < 3) {
        kloge("sys_write: invalid file handle (%d)\n", fh);
        cpu_set_errno(EPERM);
        return -1;
    }

    return vfs_write(fh, buff, count);
}