/**
 * @file faccessat.c
 * @author Zack Bostock
 * @brief Functionality pertaining to faccessat system call
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <proc/syscalls/faccessat.h>
#include <proc/syscall.h>
#include <proc/process.h>

#include <sys/smp.h>

#include <fs/vfs.h>

#include <common/kprint.h>

/**
 * @brief check user's permissions of a file relative to a directory file
 * descriptor
 *
 * @param dirfh Directory file handle
 * @param path Path to file to check
 * @param mode Permissions mode
 * @param flags Constructed using constants defined in syscall.h
 * @return int64_t
 */
int64_t sys_faccessat(int64_t dirfh, const char *path, uint64_t mode,
                      uint64_t flags) {
    (void) flags;
    cpu_set_errno(0);
    char full_path[VFS_MAX_PATH_LEN] = {0};
    if (sys_get_full_path(dirfh, path, full_path) == SYSCALL_FAIL) {
        return -1;
    }

    klogd("kfaccessat: open \"%s\" at mode %x and flags %x\n", full_path,
                                                               mode, flags);
    VFS_TNODE *tnode = vfs_path_to_node(full_path, NO_CREATE, 0);
    if (tnode) {
        uint32_t perms = tnode->inode->permissions;
        if (((mode & R_OK) && !(perms & S_IRUSR)) ||
            ((mode & W_OK) && !(perms & S_IWUSR)) ||
            ((mode & X_OK) && !(perms & S_IXUSR))) {
            cpu_set_errno(EACCES);
            return -1;
        }
        /* Should only fall through if mode & F_OK is true */
        return 0;
    } else {
        cpu_set_errno(EBADF);
        return -1;
    }
}