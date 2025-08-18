/**
 * @file write.c
 * @author Zack Bostock
 * @brief Functionality pertaining to write system call
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <stddef.h>

#include <proc/syscalls/write.h>
#include <proc/syscall.h>
#include <proc/process.h>
#include <proc/ctxsw.h>
#include <fs/vfs.h>
#include <sys/smp.h>
#include <sys/cpu.h>
#include <common/kprint.h>
#include <common/lock.h>

typedef long ssize_t;

#ifndef SSIZE_MAX
#define SSIZE_MAX ((ssize_t)(SIZE_MAX >> 1))
#endif

extern LOCK vfs_lock;

/* Cache for TTY file handle to avoid repeated open/close */
static VFS_HANDLE cached_tty_fh = VFS_INVALID_HANDLE;
static LOCK tty_cache_lock = LOCK_NEW;

/**
 * @brief Helper function to validate buffer accessibility
 *
 * @param buff Buffer pointer to validate
 * @param count Number of bytes to validate
 * @return uint8_t TRUE if valid, FALSE otherwise
 */
static uint8_t validate_user_buffer(const void *buff, size_t count) {
    if (!buff || count == 0) {
        return FALSE;
    }

    /* TODO: Add proper user space memory validation here
     * This should check if the buffer is accessible from user space
     * and doesn't cross into kernel memory regions
     */

    return TRUE;
}

/**
 * @brief Helper function to find file handle redirection
 *
 * @param pcurr Current process
 * @param fh File handle to check
 * @return VFS_HANDLE Redirected handle or VFS_INVALID_HANDLE if not found
 */
static VFS_HANDLE find_dup_handle(PROCESS *pcurr, VFS_HANDLE fh) {
    if (!pcurr) {
        return VFS_INVALID_HANDLE;
    }

    LOCK_LOCK(&vfs_lock);

    for (size_t i = 0; i < vector_len(&pcurr->dup_list); i++) {
        FILE_DUP dup = vector_at(&pcurr->dup_list, i);
        if (dup.new == fh) {
            UNLOCK_LOCK(&vfs_lock);
            return dup.prev;
        } else if (dup.prev == fh) {
            UNLOCK_LOCK(&vfs_lock);
            return dup.new;
        }
    }

    UNLOCK_LOCK(&vfs_lock);
    return VFS_INVALID_HANDLE;
}

/**
 * @brief Helper function to get TTY handle with caching
 *
 * @return VFS_HANDLE Valid TTY handle or VFS_INVALID_HANDLE on failure
 */
static VFS_HANDLE get_tty_handle(void) {
    LOCK_LOCK(&tty_cache_lock);

    if (cached_tty_fh == VFS_INVALID_HANDLE) {
        cached_tty_fh = vfs_open("/dev/tty", VFS_READ_WRITE);
        if (cached_tty_fh == VFS_INVALID_HANDLE) {
            UNLOCK_LOCK(&tty_cache_lock);
            kloge("sys_write: Failed to open /dev/tty\n");
            return VFS_INVALID_HANDLE;
        }
    }

    VFS_HANDLE result = cached_tty_fh;
    UNLOCK_LOCK(&tty_cache_lock);
    return result;
}

/**
 * @brief Helper function to write to standard streams (stdout/stderr)
 *
 * @param fh File handle (STDOUT or STDERR)
 * @param buff Buffer to write
 * @param count Number of bytes to write
 * @return int64_t Bytes written or -1 on error
 */
static int64_t write_to_standard_stream(VFS_HANDLE fh, const void *buff, size_t count) {
    PROCESS *pcurr = sched_get_curr_proc();

    /* Check for redirection first */
    VFS_HANDLE redirected_fh = find_dup_handle(pcurr, fh);
    if (redirected_fh != VFS_INVALID_HANDLE) {
        klogd("sys_write: writing %d bytes to fh %d -> redirected fh %d\n",
              count, fh, redirected_fh);
        return vfs_write(redirected_fh, buff, count);
    }

    /* Write to TTY */
    VFS_HANDLE tty_fh = get_tty_handle();
    if (tty_fh == VFS_INVALID_HANDLE) {
        cpu_set_errno(EIO);
        return -1;
    }

    int64_t result = vfs_write(tty_fh, buff, count);
    if (result < 0) {
        kloge("sys_write: Failed to write to TTY (error: %d)\n", cpu_get_errno());
    }

    return result;
}

/**
 * @brief System call implementation for writing to a file
 *
 * @param fh File handle to write to
 * @param buff Buffer of which to write
 * @param count Number of bytes to write
 * @return int64_t Bytes written, or -1 if failure
 */
int64_t sys_write(int64_t fh, void *buff, size_t count) {
    /* Input validation */
    if (fh == VFS_INVALID_HANDLE) {
        cpu_set_errno(EBADF);
        return -1;
    }

    if (!validate_user_buffer(buff, count)) {
        cpu_set_errno(EINVAL);
        return -1;
    }

    /* Handle zero-byte writes */
    if (count == 0) {
        return 0;
    }

    /* Prevent integer overflow in size calculations */
    if (count > SSIZE_MAX) {
        cpu_set_errno(EINVAL);
        return -1;
    }

    /* Clear errno before operation */
    cpu_set_errno(0);

    /* Handle standard streams */
    if (fh == STDOUT || fh == STDERR) {
        return write_to_standard_stream(fh, buff, count);
    }

    /* Handle standard input (should not be writable) */
    if (fh == STDIN) {
        kloge("sys_write: Attempt to write to stdin (fh: %d)\n", fh);
        cpu_set_errno(EBADF);
        return -1;
    }

    /* Validate file handle range */
    if (fh < 0) {
        kloge("sys_write: Invalid negative file handle (%d)\n", fh);
        cpu_set_errno(EBADF);
        return -1;
    }

    /* Perform the actual write operation */
    int64_t result = vfs_write(fh, buff, count);

    /* Log detailed error information on failure */
    if (result < 0) {
        int err = cpu_get_errno();
        kloge("sys_write: vfs_write failed for fh %d, count %d (errno: %d)\n",
              fh, count, err);
    } else if (result != (int64_t)count) {
        klogw("sys_write: Partial write for fh %d (requested: %d, written: %d)\n",
              fh, count, result);
    }

    return result;
}

/**
 * @brief Cleanup function for write subsystem
 * Should be called during system shutdown
 */
void sys_write_cleanup(void) {
    LOCK_LOCK(&tty_cache_lock);
    if (cached_tty_fh != VFS_INVALID_HANDLE) {
        vfs_close(cached_tty_fh);
        cached_tty_fh = VFS_INVALID_HANDLE;
    }
    UNLOCK_LOCK(&tty_cache_lock);
}