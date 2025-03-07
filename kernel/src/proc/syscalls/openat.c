/**
 * @file openat.c
 * @author Zack Bostock
 * @brief Functionality pertaining to openat system call
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <proc/syscalls/openat.h>
#include <proc/syscall.h>

#include <fs/vfs.h>

#include <sys/smp.h>

#include <common/kprint.h>

/**
 * @brief System call implementation for openat
 *
 * @param dirfh Directory file handle
 * @param path Path to open at
 * @param flags Flags to open with
 * @param mode Mode to open with
 * @return int64_t 0 if success, -1 if failure
 */
int64_t sys_openat(int64_t dirfh, char *path, int64_t flags, int64_t mode) {
    /* mode is always zero */
    (void) mode;
    /* Reset errno */
    cpu_set_errno(0);

    char full_path[VFS_MAX_PATH_LEN] = {0};
    if (sys_get_full_path(dirfh, path, full_path) == SYSCALL_FAIL) {
        kloge("sys_openat: cannot get full path for \"%s\"\n", path);
        cpu_set_errno(EINVAL);
        return -1;
    } else {
        /* Check whether directory exists or not */
        size_t len = strlen(full_path);
        if (len == 0) {
            kloge("sys_openat: full path of \"%s\" is NULL\n", path);
            cpu_set_errno(EINVAL);
            return -1;
        }

        /* Remove the ending slash if needed */
        for (int64_t i = len - 1; i >= 0; i--) {
            if (full_path[i] == '/') {
                full_path[i] = '\0';
                break;
            }
        }

        /* Have to recheck full_path strlen now that its been changed */
        if (strlen(full_path) > 0) {
            /* Get the tnode related to the directory */
            VFS_TNODE *tnode = vfs_path_to_node(full_path, NO_CREATE, 0);
            if (!tnode) {
                kloge("sys_openat: directory \"%s\" doesn't exist\n", full_path);
                cpu_set_errno(ENOENT);
                return -1;
            }
        }

        /* TODO: Review if this is get_full_path is still necessary */
        if (sys_get_full_path(dirfh, path, full_path) == SYSCALL_FAIL) {
            kloge("sys_openat: full path of \"%s\" cannot be determined\n", path);
            cpu_set_errno(EINVAL);
            return -1;
        }
        klogd("sys_openat: continuing opening \"%s\"\n", full_path);
    }

    VFS_OPEN_MODE omode = VFS_READ_WRITE;
    int32_t perms = 0;
    switch (flags & 0x7) {
        case O_EXEC:
            omode = VFS_READ;
            perms = S_IRUSR | S_IXUSR;
            break;
        case O_RDONLY:
            omode = VFS_READ;
            perms = S_IRUSR;
            break;
        case O_WRONLY:
            omode = VFS_WRITE;
            perms = S_IRUSR;
            break;
        case O_RDWR:
        default:
            omode = VFS_READ_WRITE;
            perms = S_IRUSR | S_IWUSR;
            break;
    }

    if (flags & O_CREAT) {
        int64_t ret = vfs_create(full_path, VFS_FILE);
        if (ret < 0) {
            kloge("sys_openat: creating file for \"%s\" failed\n", path);
            cpu_set_errno(EEXIST);
            return ret;
        } else {
            VFS_HANDLE fh = vfs_open(full_path, VFS_WRITE);
            if (fh != VFS_INVALID_HANDLE) {
                vfs_chmod(fh, perms | S_IRUSR);
                vfs_close(fh);
            }
        }
    }

    klogd("sys_openat: dirfh %x, path %s and flags %x\n", dirfh, path, flags);
    return vfs_open(full_path, omode);
}
