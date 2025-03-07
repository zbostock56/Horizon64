/**
 * @file seek.c
 * @author Zack Bostock
 * @brief Functionality pertaining to seek system call
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <proc/syscalls/seek.h>
#include <proc/syscall.h>

#include <fs/vfs.h>

#include <sys/smp.h>

#include <common/kprint.h>

/**
 * @brief System call implementation of seeking on a file
 *
 * @param fh File handle to seek
 * @param offset Offset to start from
 * @param whence To whence to seek
 * @return int64_t -1 if failure, otherwise returns resulting offset location in
 * file measured from the beginning of the file
 */
int64_t sys_seek(int64_t fh, int64_t offset, int64_t whence) {
    /* Reset errno */
    cpu_set_errno(0);

    if (fh == STDIN || fh == STDOUT || fh == STDERR) {
        klogv("sys_seek: fh %d, offset %d, whence %d\n", fh, offset, whence);
        return 0;
    }

    int64_t ret = vfs_seek(fh, offset, whence);

    klogd("sys_seek: fh %d, offset %d, whence %d with return value %d\n",
          fh, offset, whence, ret);
    if (ret < 0) {
        cpu_set_errno(EINVAL);
    }

    return ret;
}
