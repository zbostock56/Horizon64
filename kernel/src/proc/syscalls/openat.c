/**
 * @file openat.c
 * @author Zack Bostock
 * @brief Functionality pertaining to openat system call
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <proc/syscalls/openat.h>
#include <proc/syscall.h>
#include <proc/process.h>
#include <fs/vfs.h>
#include <sys/smp.h>
#include <sys/cpu.h>
#include <common/kprint.h>
#include <common/string.h>

/* Maximum recursion depth for symlink resolution */
#define MAX_SYMLINK_DEPTH 8

/**
 * @brief Validate user-provided path string
 *
 * @param path Path string to validate
 * @return uint8_t TRUE if valid, FALSE otherwise
 */
static uint8_t validate_path_string(const char *path) {
    if (!path) {
        return FALSE;
    }

    /* TODO: Add proper user space memory validation
     * Should verify the string is accessible from user space
     * and doesn't cross into kernel memory
     */

    /* Basic length validation */
    size_t len = strnlen(path, VFS_MAX_PATH_LEN + 1);
    if (len == 0 || len > VFS_MAX_PATH_LEN) {
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Validate file creation flags combination
 *
 * @param flags Open flags to validate
 * @return uint8_t TRUE if valid combination, FALSE otherwise
 */
static uint8_t validate_open_flags(int64_t flags) {
    /* Extract access mode */
    int access_mode = flags & O_ACCMODE;

    /* Validate access mode is one of the valid values */
    if (access_mode != O_RDONLY && access_mode != O_WRONLY &&
        access_mode != O_RDWR && access_mode != O_EXEC) {
        return FALSE;
    }

    /* O_CREAT with O_EXCL should not succeed if file exists */
    /* O_TRUNC without write access doesn't make sense */
    if ((flags & O_TRUNC) && access_mode == O_RDONLY) {
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Convert open flags to VFS open mode
 *
 * @param flags Open flags
 * @return VFS_OPEN_MODE Corresponding VFS mode
 */
static VFS_OPEN_MODE flags_to_vfs_mode(int64_t flags) {
    int access_mode = flags & O_ACCMODE;

    switch (access_mode) {
        case O_EXEC:        /* 0x1 */
            return VFS_READ; /* Execute needs read access */
        case O_RDONLY:      /* 0x2 */
            return VFS_READ;
        case O_RDWR:        /* 0x3 */
            return VFS_READ_WRITE;
        case O_SEARCH:      /* 0x4 */
            return VFS_READ; /* Search needs read access */
        case O_WRONLY:      /* 0x5 */
            return VFS_WRITE;
        default:
            /* For safety, default to read-only for unknown modes */
            return VFS_READ;
    }
}

/**
 * @brief Convert open flags to permission bits
 *
 * @param flags Open flags
 * @param mode User-specified mode (if O_CREAT)
 * @return int32_t Permission bits
 */
static int32_t flags_to_permissions(int64_t flags, int64_t mode) {
    int32_t perms = 0;

    /* If mode is provided (O_CREAT), use it as base */
    if (flags & O_CREAT && mode != 0) {
        perms = (int32_t)(mode & 0777); /* Mask to permission bits */
    } else {
        /* Default permissions based on access mode */
        switch (flags & O_ACCMODE) {
            case O_EXEC:
                perms = S_IRUSR | S_IXUSR;
                break;
            case O_RDONLY:
                perms = S_IRUSR;
                break;
            case O_WRONLY:
                perms = S_IWUSR;
                break;
            case O_RDWR:
            default:
                perms = S_IRUSR | S_IWUSR;
                break;
        }
    }

    return perms;
}

/**
 * @brief Handle file creation with proper error checking
 *
 * @param full_path Full path to create
 * @param flags Open flags
 * @param mode File mode
 * @return int64_t 0 on success, -1 on error (errno set)
 */
static int64_t handle_file_creation(const char *full_path, int64_t flags, int64_t mode) {
    /* Check if file already exists when O_EXCL is specified */
    if (flags & O_EXCL) {
        VFS_TNODE *existing = vfs_path_to_node(full_path, NO_CREATE, 0);
        if (existing) {
            kloge("sys_openat: File \"%s\" already exists with O_EXCL\n", full_path);
            cpu_set_errno(EEXIST);
            return -1;
        }
    }

    /* Attempt to create the file */
    int result = vfs_create(full_path, VFS_FILE);
    if (result == SYS_ERR) {
        /* Check if failure was due to existing file (and O_EXCL not set) */
        if (!(flags & O_EXCL)) {
            VFS_TNODE *existing = vfs_path_to_node(full_path, NO_CREATE, 0);
            if (existing) {
                /* File exists, this is fine without O_EXCL */
                return 0;
            }
        }

        kloge("sys_openat: Failed to create file \"%s\"\n", full_path);
        cpu_set_errno(EACCES); /* Could be permissions, disk full, etc. */
        return -1;
    }

    /* Set file permissions if created successfully */
    int32_t perms = flags_to_permissions(flags, mode);
    VFS_HANDLE temp_fh = vfs_open(full_path, VFS_WRITE);
    if (temp_fh != VFS_INVALID_HANDLE) {
        vfs_chmod(temp_fh, perms);
        vfs_close(temp_fh);
    } else {
        klogw("sys_openat: Created file \"%s\" but couldn't set permissions\n",
                full_path);
    }

    return 0;
}

/**
 * @brief Normalize path by removing trailing slashes and resolving components
 *
 * @param path Input path
 * @param normalized Output buffer for normalized path
 * @param max_len Maximum length of output buffer
 * @return int64_t 0 on success, -1 on error
 */
static int64_t normalize_path(const char *path, char *normalized, size_t max_len) {
    size_t len = strlen(path);
    if (len >= max_len) {
        return -1;
    }

    strncpy(normalized, path, max_len - 1);
    normalized[max_len - 1] = '\0';

    /* Remove trailing slashes (except for root) */
    len = strlen(normalized);
    while (len > 1 && normalized[len - 1] == '/') {
        normalized[len - 1] = '\0';
        len--;
    }

    return 0;
}

/**
 * @brief System call implementation for openat
 *
 * @param dirfh Directory file handle (AT_FDCWD for current directory)
 * @param path Path to open (relative to dirfh or absolute)
 * @param flags Open flags (O_RDONLY, O_WRONLY, O_RDWR, etc.)
 * @param mode File mode (used with O_CREAT)
 * @return int64_t File handle on success, -1 on failure
 */
int64_t sys_openat(int64_t dirfh, char *path, int64_t flags, int64_t mode) {
    if (!validate_path_string(path)) {
        kloge("sys_openat: Invalid path parameter\n");
        cpu_set_errno(EINVAL);
        return -1;
    }

    if (!validate_open_flags(flags)) {
        kloge("sys_openat: Invalid flags combination: %x\n", flags);
        cpu_set_errno(EINVAL);
        return -1;
    }

    /* Validate directory file handle */
    if (dirfh != AT_FDCWD && dirfh < 0) {
        kloge("sys_openat: Invalid directory file handle: %d\n", dirfh);
        cpu_set_errno(EBADF);
        return -1;
    }

    /* Reset errno */
    cpu_set_errno(0);

    /* Resolve full path */
    char full_path[VFS_MAX_PATH_LEN] = {0};
    if (sys_get_full_path(dirfh, path, full_path) == SYSCALL_FAIL) {
        kloge("sys_openat: Cannot resolve full path for \"%s\" (dirfh: %d)\n",
              path, dirfh);
        cpu_set_errno(ENOENT);
        return -1;
    }

    /* Normalize the path */
    char normalized_path[VFS_MAX_PATH_LEN] = {0};
    if (normalize_path(full_path, normalized_path, sizeof(normalized_path)) < 0) {
        kloge("sys_openat: Path too long: \"%s\"\n", full_path);
        cpu_set_errno(ENAMETOOLONG);
        return -1;
    }

    klogd("sys_openat: Opening \"%s\" with flags %x, mode %x\n",
          normalized_path, flags, mode);

    /* Handle file creation if requested */
    if (flags & O_CREAT) {
        if (handle_file_creation(normalized_path, flags, mode) < 0) {
            /* errno already set */
            return -1;
        }
    }

    /* Check if target exists (required unless O_CREAT) */
    VFS_TNODE *tnode = vfs_path_to_node(normalized_path, NO_CREATE, 0);
    if (!tnode) {
        if (flags & O_CREAT) {
            /* File should have been created above, try again */
            tnode = vfs_path_to_node(normalized_path, NO_CREATE, 0);
        }

        if (!tnode) {
            kloge("sys_openat: Path \"%s\" does not exist\n", normalized_path);
            cpu_set_errno(ENOENT);
            return -1;
        }
    }

    /* TODO: Check if trying to open directory with write access */
    /* Stopped right now because of `ls` issues */
    /*
    if (tnode->inode->type == VFS_DIRECTORY && (flags & (O_WRONLY | O_RDWR))) {
        kloge("sys_openat: Cannot open directory \"%s\" for writing\n",
                normalized_path);
        cpu_set_errno(EISDIR);
        return -1;
    }
    */

    /* Convert flags to VFS mode */
    VFS_OPEN_MODE vfs_mode = flags_to_vfs_mode(flags);

    /* Open the file */
    VFS_HANDLE fh = vfs_open(normalized_path, vfs_mode);
    if (fh == VFS_INVALID_HANDLE) {
        int vfs_errno = cpu_get_errno();
        kloge("sys_openat: Failed to open \"%s\" (VFS errno: %d)\n",
              normalized_path, vfs_errno);

        /* Map VFS errors to appropriate errno values */
        if (vfs_errno == 0) {
            /* Generic I/O error if no specific error */
            cpu_set_errno(EIO);
        }
        /* Otherwise, keep the errno set by VFS */

        return -1;
    }

    if ((flags & O_TRUNC) && (flags & (O_WRONLY | O_RDWR))) {
        /* TODO: Handle O_TRUNC flag */
        klogw("sys_openat: O_TRUNC not yet supported\n");
    }

    klogd("sys_openat: Successfully opened \"%s\" as handle %d\n",
          normalized_path, fh);

    return fh;
}