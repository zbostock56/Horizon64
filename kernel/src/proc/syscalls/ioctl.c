/**
 * @file ioctl.c
 * @author Zack Bostock
 * @brief Functionality pertaining to ioctl system call
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <proc/syscalls/ioctl.h>
#include <proc/syscall.h>

#include <fs/vfs.h>

#include <common/kprint.h>

#include <sys/cpu.h>
#include <sys/smp.h>

/**
 * @brief System call implemenation for ioctl
 *
 * @param fh File handle to control
 * @param request Request for what to do
 * @param arg Arguments that correspond with the request
 * @return int64_t -1 if failure, otherwise value is returned from underlying
 * device
 */
int64_t sys_ioctl(int64_t fh, int64_t request, int64_t arg) {
    cpu_set_errno(0);

    if (fh == STDIN || fh == STDOUT || fh == STDERR) {
        VFS_HANDLE ttyfh = vfs_open("/dev/tty", VFS_READ_WRITE);
        if (ttyfh != VFS_INVALID_HANDLE) {
            int64_t ret = vfs_ioctl(ttyfh, request, arg);
            vfs_close(ttyfh);
            return ret;
        }
    }

    cpu_set_errno(EINVAL);
    return -1;
}