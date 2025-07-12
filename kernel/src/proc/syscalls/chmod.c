/**
 * @file chmod.c
 * @author Zack Bostock
 * @brief Functionality pertaining to chmod system call
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <proc/syscalls/chmod.h>
#include <proc/syscall.h>
#include <proc/process.h>
#include <proc/ctxsw.h>
#include <proc/syscalls/openat.h>

#include <sys/smp.h>

#include <common/kprint.h>

/**
 * @brief Changes permissions
 *
 * @param path Path of file to change permissions for
 * @param flags Flags of permissions
 * @return int64_t 0 if success, -1 if failure
 */
int64_t sys_chmod(char *path, int64_t flags) {
    cpu_set_errno(0);

    klogd("sys_chmod: \"%s\" changing permissions with flags %x\n", path, flags);

    VFS_HANDLE fh = sys_openat(VFS_FW_CWD, path, O_RDWR, 0);
    if (fh != VFS_INVALID_HANDLE) {
        int32_t perms = 0;

        switch (flags & 0x7) {
            case O_EXEC:
                perms = S_IRUSR | S_IXUSR;
                break;
            case O_RDONLY:
                perms = S_IRUSR;
                break;
            case O_WRONLY:
                perms = S_IRUSR;
                break;
            case O_RDWR:
            default:
                perms = S_IRUSR | S_IWUSR;
                break;
        }

        if (vfs_chmod(fh, perms | S_IRUSR) != SYS_OK) {
            klogw("sys_chmod: vfs_chmod returned failure, check errno\n");
            return -1;
        }
        if (vfs_close(fh) != 0) {
            klogw("sys_chmod: vfs_close returned failure, check errno\n");
            return -1;
        }
        return 0;
    }

    cpu_set_errno(ENOENT);
    return -1;
}