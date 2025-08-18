/**
 * @file ioctl.c
 * @author Zack Bostock
 * @brief Functionality pertaining to ioctl system call
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <proc/syscalls/ioctl.h>
#include <proc/syscall.h>
#include <proc/process.h>
#include <fs/vfs.h>
#include <common/kprint.h>
#include <sys/cpu.h>
#include <sys/smp.h>

/**
 * @brief System call implementation for ioctl
 *
 * @param fd File descriptor to control
 * @param request Request code specifying the operation
 * @param arg Arguments that correspond with the request
 * @return int64_t -1 if failure (errno set), otherwise value returned from device
 */
int64_t sys_ioctl(int64_t fd, int64_t request, int64_t arg) {
    cpu_set_errno(0);
    
    if (request < 0) {
        cpu_set_errno(EINVAL);
        return -1;
    }
    
    if (fd == STDIN || fd == STDOUT || fd == STDERR) {
        VFS_HANDLE ttyfh = vfs_open("/dev/tty", VFS_READ_WRITE);
        if (ttyfh != VFS_INVALID_HANDLE) {
            int64_t result = vfs_ioctl(ttyfh, request, arg);
            if (result < 0) {
                cpu_set_errno(-result);
                return -1;
            }
            return result;
        }
    }
    cpu_set_errno(EINVAL);
    return -1;
}