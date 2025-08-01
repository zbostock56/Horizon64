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
#include <common/string.h>

/**
 * @brief Validate mkdirat parameters
 *
 * @param dirfh Directory file handle
 * @param pathname Path to create directory at
 * @param mode Directory mode/permissions
 * @return int 0 on success, negative errno on failure
 */
static int validate_mkdirat_params(int64_t dirfh, const char *pathname, uint64_t mode) {
    /* Check for null pathname */
    if (!pathname) {
        return -EFAULT;
    }

    /* Check pathname length */
    size_t path_len = strlen(pathname);
    if (path_len == 0) {
        return -ENOENT;
    }
    if (path_len >= VFS_MAX_PATH_LEN) {
        return -ENAMETOOLONG;
    }

    /* Validate file descriptor */
    if (dirfh != AT_FDCWD && dirfh == VFS_INVALID_HANDLE) {
        return -EBADF;
    }

    /* TODO: Validate mode */
    (void)mode;

    return 0;
}

/**
 * @brief Check if user has permission to create directory in parent
 *
 * @param parent_path Parent directory path
 * @return int 0 on success, negative errno on failure
 */
static int check_parent_permissions(const char *parent_path) {
    VFS_TNODE *parent_node = vfs_path_to_node(parent_path, CREATE, VFS_DIRECTORY);
    if (!parent_node) {
        return -ENOENT;
    }

    /* If we got here, parent exists and we have access to it */
    /* TODO: should check permissions here */
    /* For now, just assume that if vfs_path_to_node works, its fine */

    return 0;
}

/**
 * @brief Get parent directory path from full path
 *
 * @param full_path Full path to directory
 * @param parent_path Buffer to store parent path
 * @param parent_size Size of parent path buffer
 * @return int 0 on success, negative errno on failure
 */
static int get_parent_path(const char *full_path, char *parent_path, size_t parent_size) {
    size_t path_len = strlen(full_path);
    if (path_len >= parent_size) {
        return -ENAMETOOLONG;
    }

    strcpy(parent_path, full_path);

    /* Find last '/' to get parent directory */
    char *last_slash = strrchr(parent_path, '/');
    if (!last_slash) {
        /* No parent directory found */
        return -EINVAL;
    }

    /* Handle root directory case */
    if (last_slash == parent_path) {
        parent_path[1] = '\0';
    } else {
        *last_slash = '\0';
    }

    return 0;
}

/**
 * @brief System call implementation to create a directory
 *
 * @param dirfh Directory file handle (AT_FDCWD for current directory)
 * @param pathname Relative or absolute path to create directory at
 * @param mode Mode/permissions to be set on the directory
 * @return int64_t 0 if success, -errno if failure
 */
int64_t sys_mkdirat(int64_t dirfh, const char *pathname, uint64_t mode) {
    int ret;
    char full_path[VFS_MAX_PATH_LEN] = {0};
    char parent_path[VFS_MAX_PATH_LEN] = {0};
    VFS_TNODE *tnode = NULL;

    /* TODO: fix only 32 bit values being passed in from usermode (sys.c) */
    if ((int32_t)dirfh == AT_FDCWD) {
        dirfh = AT_FDCWD;
    }

    /* Clear errno at start */
    cpu_set_errno(0);

    /* Validate input parameters */
    ret = validate_mkdirat_params(dirfh, pathname, mode);
    if (ret < 0) {
        cpu_set_errno(-ret);
        return -1;
    }

    /* Currently only support AT_FDCWD */
    if (dirfh != AT_FDCWD) {
        klogw("sys_mkdirat: dirfh other than AT_FDCWD not yet supported\n");
        cpu_set_errno(ENOSYS);
        return -1;
    }

    /* Get full path */
    ret = sys_get_full_path(dirfh, pathname, full_path);
    if (ret == SYSCALL_FAIL) {
        kloge("sys_mkdirat: Failed to resolve path \"%s\"\n", pathname);
        cpu_set_errno(ENOENT);
        return -1;
    }

    klogd("sys_mkdirat: Creating directory at \"%s\" with mode 0x%3x\n",
          full_path, (unsigned)mode);

    /* Check if directory already exists by trying to create it first
     * If it exists, vfs_path_to_node with CREATE should fail appropriately */

    /* Get parent directory and check permissions */
    ret = get_parent_path(full_path, parent_path, sizeof(parent_path));
    if (ret < 0) {
        kloge("sys_mkdirat: Failed to determine parent directory\n");
        cpu_set_errno(-ret);
        return -1;
    }

    ret = check_parent_permissions(parent_path);
    if (ret < 0) {
        kloge("sys_mkdirat: Permission denied for parent directory \"%s\"\n",
              parent_path);
        cpu_set_errno(-ret);
        return -1;
    }

    /* Create the directory */
    tnode = vfs_path_to_node(full_path, CREATE, VFS_DIRECTORY);
    if (!tnode) {
        kloge("sys_mkdirat: Failed to create directory \"%s\"\n", full_path);
        /* Set appropriate errno - could be EEXIST, EIO, ENOSPC, etc. */
        int64_t errno = cpu_get_errno();
        if (errno == 0) {
            cpu_set_errno(EIO);
        }
        return -1;
    }

    /* Directory created successfully */
    /* TODO: Set with proper permissions */
    if (mode != 0) {
        klogd("sys_mkdirat: Mode 0x%3x requested but mode setting not implemented\n",
              (unsigned)mode);
    }

    klogd("sys_mkdirat: Successfully created directory \"%s\"\n", full_path);
    return 0;
}