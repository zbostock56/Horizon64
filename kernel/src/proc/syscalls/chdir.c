/**
 * @file chdir.c
 * @author Zack Bostock
 * @brief Functionality pertaining to chdir system call
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <proc/syscalls/chdir.h>
#include <proc/syscall.h>
#include <proc/process.h>
#include <proc/ctxsw.h>
#include <fs/vfs.h>
#include <sys/cpu.h>
#include <sys/smp.h>
#include <string.h>
#include <common/kprint.h>
#include <common/lock.h>

/* Maximum depth for symlink resolution */
#define MAX_SYMLINK_DEPTH 8
/* Maximum number of path components */
#define MAX_PATH_COMPONENTS 256

/**
 * @brief Validate user-provided directory path
 *
 * @param dir Directory path to validate
 * @return uint8_t TRUE if valid, FALSE otherwise
 */
static uint8_t validate_directory_path(const char *dir) {
    if (!dir) {
        return FALSE;
    }

    /* TODO: Add proper user space memory validation
     * Should verify the string is accessible from user space
     * and doesn't cross into kernel memory
     */

    /* Check path length */
    size_t len = strnlen(dir, VFS_MAX_PATH_LEN + 1);
    if (len == 0 || len > VFS_MAX_PATH_LEN) {
        return FALSE;
    }

    /* Check for null bytes in the middle of the path */
    for (size_t i = 0; i < len; i++) {
        if (dir[i] == '\0') {
            break; /* Normal string termination */
        }
        /* Could add additional character validation here */
    }

    return TRUE;
}

/**
 * @brief Normalize a path component by removing leading/trailing whitespace
 *
 * @param component Path component to normalize
 * @param normalized Output buffer for normalized component
 * @param max_len Maximum length of output buffer
 * @return int64_t Length of normalized component, -1 on error
 */
static int64_t normalize_path_component(const char *component, char *normalized, size_t max_len) {
    if (!component || !normalized || max_len == 0) {
        return -1;
    }

    const char *start = component;
    const char *end = component + strlen(component);

    /* Skip leading whitespace */
    while (start < end && (*start == ' ' || *start == '\t')) {
        start++;
    }

    /* Skip trailing whitespace */
    while (end > start && (*(end-1) == ' ' || *(end-1) == '\t')) {
        end--;
    }

    size_t len = end - start;
    if (len >= max_len) {
        return -1; /* Component too long */
    }

    if (len > 0) {
        memcpy(normalized, start, len);
    }
    normalized[len] = '\0';

    return (int64_t)len;
}

/**
 * @brief Resolve a single path component against current directory
 *
 * @param current_path Current working directory (modified in place)
 * @param component Path component to resolve (".", "..", or directory name)
 * @return int 0 on success, -1 on error
 */
static int resolve_path_component(char *current_path, const char *component) {
    if (!current_path || !component) {
        return -1;
    }

    size_t comp_len = strlen(component);
    if (comp_len == 0) {
        return 0; /* Empty component, nothing to do */
    }

    /* Handle current directory "." */
    if (comp_len == 1 && component[0] == '.') {
        return 0; /* Stay in current directory */
    }

    /* Handle parent directory ".." */
    if (comp_len == 2 && component[0] == '.' && component[1] == '.') {
        /* Find last '/' in current path */
        char *last_slash = strrchr(current_path, '/');
        if (last_slash && last_slash != current_path) {
            /* Truncate at last slash (but keep it if it's root) */
            *last_slash = '\0';
        } else if (last_slash == current_path) {
            /* We're at root, stay at root */
            current_path[1] = '\0';
        }
        return 0;
    }

    /* Handle regular directory name */
    size_t current_len = strlen(current_path);

    /* Check if we need to add a slash */
    uint8_t need_slash = (current_len > 0 && current_path[current_len - 1] != '/');

    /* Check if resulting path would be too long */
    size_t new_len = current_len + (need_slash ? 1 : 0) + comp_len;
    if (new_len >= VFS_MAX_PATH_LEN) {
        cpu_set_errno(ENAMETOOLONG);
        return -1;
    }

    /* Append component to current path */
    if (need_slash) {
        strcat(current_path, "/");
    }
    strcat(current_path, component);

    return 0;
}

/**
 * @brief Parse and resolve a directory path
 *
 * @param dir Input directory path
 * @param current_cwd Current working directory
 * @param resolved_path Output buffer for resolved path
 * @return int 0 on success, -1 on error
 */
static int resolve_directory_path(const char *dir, const char *current_cwd, char *resolved_path) {
    char normalized_dir[VFS_MAX_PATH_LEN];
    char temp_component[VFS_MAX_PATH_LEN];

    /* Normalize input directory path */
    if (normalize_path_component(dir, normalized_dir, sizeof(normalized_dir)) < 0) {
        cpu_set_errno(ENAMETOOLONG);
        return -1;
    }

    /* Initialize resolved path */
    if (normalized_dir[0] == '/') {
        /* Absolute path */
        strcpy(resolved_path, "/");
    } else {
        /* Relative path - start with current working directory */
        if (strlen(current_cwd) >= VFS_MAX_PATH_LEN) {
            cpu_set_errno(ENAMETOOLONG);
            return -1;
        }
        strcpy(resolved_path, current_cwd);
    }

    /* Parse path components */
    const char *start = normalized_dir;
    const char *end;
    size_t component_count = 0;

    /* Skip leading slash for absolute paths */
    if (*start == '/') {
        start++;
    }

    while (*start != '\0' && component_count < MAX_PATH_COMPONENTS) {
        /* Find end of current component */
        end = strchr(start, '/');
        if (!end) {
            end = start + strlen(start);
        }

        /* Extract component */
        size_t comp_len = end - start;
        if (comp_len >= sizeof(temp_component)) {
            cpu_set_errno(ENAMETOOLONG);
            return -1;
        }

        memcpy(temp_component, start, comp_len);
        temp_component[comp_len] = '\0';

        /* Skip empty components (double slashes) */
        if (comp_len > 0) {
            if (resolve_path_component(resolved_path, temp_component) < 0) {
                return -1; /* errno already set */
            }
            component_count++;
        }

        /* Move to next component */
        start = (*end == '/') ? end + 1 : end;
    }

    if (component_count >= MAX_PATH_COMPONENTS) {
        kloge("sys_chdir: Path has too many components (>%d)\n", MAX_PATH_COMPONENTS);
        cpu_set_errno(ENAMETOOLONG);
        return -1;
    }

    /* Ensure path ends properly */
    size_t final_len = strlen(resolved_path);
    if (final_len == 0) {
        strcpy(resolved_path, "/");
    } else if (final_len > 1 && resolved_path[final_len - 1] == '/') {
        /* Remove trailing slash unless it's root */
        resolved_path[final_len - 1] = '\0';
    }

    return 0;
}

/**
 * @brief Verify that target path exists and is a directory
 *
 * @param path Path to verify
 * @return VFS_TNODE* Directory tnode on success, NULL on error
 */
static VFS_TNODE* verify_target_directory(const char *path) {
    VFS_TNODE *tnode = vfs_path_to_node(path, NO_CREATE, 0);
    if (!tnode) {
        cpu_set_errno(ENOENT);
        return NULL;
    }

    /* Verify it's a directory */
    if (tnode->inode->type != VFS_DIRECTORY &&
        tnode->inode->type != VFS_MOUNT_POINT) {
        kloge("sys_chdir: \"%s\" is not a directory\n", path);
        cpu_set_errno(ENOTDIR);
        return NULL;
    }

    /* TODO: Add permission checking here
     * Should verify the process has execute (search) permission on the directory
     * - Process UID/GID against directory owner/group
     * - Directory permission bits (especially execute bit)
     */

    return tnode;
}

/**
 * @brief Update process current working directory atomically
 *
 * @param pcurr Process to update
 * @param new_cwd New current working directory
 * @return int 0 on success, -1 on error
 */
static int update_process_cwd(PROCESS *pcurr, const char *new_cwd) {
    if (!pcurr || !new_cwd) {
        return -1;
    }

    size_t new_len = strlen(new_cwd);
    if (new_len >= VFS_MAX_PATH_LEN) {
        cpu_set_errno(ENAMETOOLONG);
        return -1;
    }

    /* TODO: Add per-process lock
     * LOCK_LOCK(&pcurr->lock);
     */

    /* Store old CWD for potential rollback */
    char old_cwd[VFS_MAX_PATH_LEN];
    strcpy(old_cwd, pcurr->cwd);

    /* Update current working directory */
    strcpy(pcurr->cwd, new_cwd);

    klogd("sys_chdir: Process %d changed directory from \"%s\" to \"%s\"\n",
          pcurr->id, old_cwd, new_cwd);

    /* TODO: Unlock process if locked above
     * UNLOCK_LOCK(&pcurr->lock);
     */

    return 0;
}

/**
 * @brief System call implementation to change directory
 *
 * Changes the current working directory of the calling process to the
 * directory specified by the path. The path can be absolute or relative
 * to the current working directory.
 *
 * @param dir Directory path to change to (null-terminated string)
 * @return int64_t 0 on success, -1 on failure (errno is set)
 */
int64_t sys_chdir(char *dir) {
    /* Input validation */
    if (!validate_directory_path(dir)) {
        kloge("sys_chdir: Invalid directory path parameter\n");
        cpu_set_errno(EINVAL);
        return -1;
    }

    /* Get current process */
    PROCESS *pcurr = sched_get_curr_proc();
    if (!pcurr) {
        kloge("sys_chdir: No current process context\n");
        cpu_set_errno(ENODEV);
        return -1;
    }

    /* Validate process state */
    if (pcurr->id < 1) {
        kloge("sys_chdir: Invalid process ID %d\n", pcurr->id);
        cpu_set_errno(ESRCH);
        return -1;
    }

    /* Clear errno */
    cpu_set_errno(0);

    /* Resolve the target directory path */
    char resolved_path[VFS_MAX_PATH_LEN];
    if (resolve_directory_path(dir, pcurr->cwd, resolved_path) < 0) {
        kloge("sys_chdir: Failed to resolve path \"%s\"\n", dir);
        return -1; /* errno already set */
    }

    klogd("sys_chdir: Resolved \"%s\" to \"%s\" (from cwd \"%s\")\n",
          dir, resolved_path, pcurr->cwd);

    /* Verify target directory exists and is accessible */
    VFS_TNODE *target_tnode = verify_target_directory(resolved_path);
    if (!target_tnode) {
        kloge("sys_chdir: Cannot access directory \"%s\"\n", resolved_path);
        return -1; /* errno already set */
    }

    /* Update process current working directory */
    if (update_process_cwd(pcurr, resolved_path) < 0) {
        kloge("sys_chdir: Failed to update process CWD\n");
        return -1; /* errno already set */
    }

    klogd("sys_chdir: Successfully changed to directory \"%s\"\n", resolved_path);
    return 0;
}