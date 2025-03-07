/**
 * @file read.c
 * @author Zack Bostock
 * @brief Functionality pertaining to read system call
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <proc/syscalls/read.h>
#include <proc/syscall.h>
#include <proc/process.h>
#include <proc/ctxsw.h>

#include <fs/vfs.h>

#include <sys/smp.h>

#include <common/kprint.h>

extern LOCK vfs_lock;

/**
 * @brief System call implementation for reading from a file
 *
 * @param fh File handle to read from
 * @param buff Buffer to copy contents into
 * @param count Number of bytes to read
 * @return int64_t Number of bytes read or -1 if failure
 */
int64_t sys_read(int64_t fh, void *buff, size_t count) {
    if (!buff || count == 0 || fh == VFS_INVALID_HANDLE) {
        cpu_set_errno(EINVAL);
        return -1;
    }

    PROCESS *pcurr = sched_get_curr_proc();

    /* Reset errno */
    cpu_set_errno(0);

    klogd("sys_read: reading %d bytes from file handle %d\n", count, fh);

    if (fh == STDIN) {
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
                }
            }
            UNLOCK_LOCK(&vfs_lock);
        }

        if (found) {
            int64_t ret = vfs_read(oldfh, count, buff);
            klogd("sys_read: reda from handle %d instead of %d and read %d bytes",
                  oldfh, fh, ret);
            return ret;
        } else {
            VFS_HANDLE ttyfh = vfs_open("/dev/tty", VFS_READ_WRITE);
            if (ttyfh != VFS_INVALID_HANDLE) {
                int64_t len = vfs_read(ttyfh, count, buff);
                vfs_close(ttyfh);
                return len;
            }
        }
        cpu_set_errno(EINVAL);
        return -1;
    } else if (fh >= VFS_MIN_HANDLE) {
        int64_t len = vfs_read(fh, count, buff);
        klogd("sys_read: tried to read %d bytes from file %d and read %d bytes\n",
              count, fh, len);
        return len;
    } else {
        cpu_set_errno(EBADF);
        return -1;
    }
}
