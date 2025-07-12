/**
 * @file mkdirat.c
 * @author Zack Bostock
 * @brief Functionality pertaining to mkdirat system call
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <proc/syscalls/mkdirat.h>
#include <proc/process.h>
#include <proc/syscall.h>

#include <fs/vfs.h>

#include <sys/cpu.h>
#include <sys/smp.h>

#include <common/kprint.h>

#define AT_FDCWD        (-100)

/**
 * @brief System call implementation to create a directory
 *
 * @param dirfh Directory file handle
 * @param pathname Relative or absolute path to create directory at
 * @param mode Mode on information to be set about the directory
 * @return int64_t 0 if success, -1 if failure
 */
int64_t sys_mkdirat(int64_t dirfh, const char *pathname, uint64_t mode) {
    /* Unused currently */
    (void) mode;
    cpu_set_errno(0);

    if (dirfh == VFS_INVALID_HANDLE) {
        cpu_set_errno(EBADF);
        return -1;
    } else if (!pathname) {
        cpu_set_errno(EINVAL);
        return -1;
    } else if (dirfh != AT_FDCWD) {
        kloge("sys_mkdirat: Does not yet support creating directories away"
              " from current directory\n");
        cpu_set_errno(EINVAL);
        return -1;
    }

    char full_path[VFS_MAX_PATH_LEN] = {0};
    if (sys_get_full_path(dirfh, pathname, full_path) == SYSCALL_FAIL) {
        kloge("sys_mkdirat: Failed to get full path of \"%s\"\n", pathname);
        cpu_set_errno(EINVAL);
    }

    klogd("sys_mkdirat: Got full path of \"%s\" as \"%s\"\n", pathname, full_path);

    VFS_TNODE *tnode = vfs_path_to_node(full_path, CREATE, VFS_DIRECTORY);
    if (!tnode) {
        kloge("sys_mkdirat: Failed to create directory at \"%s\"\n", pathname);
        return -1;
    }

    /* Permissions are already set in vfs_path_to_node */

    return 0;
}
