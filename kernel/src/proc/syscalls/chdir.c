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

#include <common/string.h>
#include <common/kprint.h>

/**
 * @brief System call implementation to change directory
 *
 * @param dir Directory to change to
 * @return int64_t 0 if success, -1 if failure
 */
int64_t sys_chdir(char *dir) {
    if (!dir) {
        cpu_set_errno(EINVAL);
        return -1;
    }

    PROCESS *pcurr = sched_get_curr_proc();
    cpu_set_errno(0);

    /* TODO: need to add bounds check */
    while (*dir == ' ') {
        dir++;
    }

    if (strlen(dir) == 0) {
        cpu_set_errno(ENOENT);
        return -1;
    } else if (!pcurr) {
        cpu_set_errno(ENODEV);
        return -1;
    } else if (pcurr->id < 1) {
        cpu_set_errno(ESRCH);
        return -1;
    }

    char full_path[VFS_MAX_PATH_LEN] = {0};
    char parent[VFS_MAX_PATH_LEN] = {0};
    char currdir[VFS_MAX_PATH_LEN] = {0};

    size_t k = 0;
    size_t len = strlen(dir);
    strcpy(full_path, pcurr->cwd);

    /* Note that 'i' will loop to the last character '\0' */
    for (size_t i = 0; i < len; i++) {
        if (dir[i] != '/') {
            currdir[k++] = dir[i];
            if (i != len - 1) {
                continue;
            }
        }
        currdir[k] = '\0';

        /* currdir stores current directory name */
        if (strcmp(currdir, ".") == 0) {
            /* Trying to change directory into current directory, do nothing */
        } else if (strcmp(currdir, "..") == 0) {
            /* Trying to change to parent directory */
            if (vfs_get_parent_dir(full_path, parent, currdir) < 0) {
                cpu_set_errno(EINVAL);
                return -1;
            }
            strcpy(full_path, parent);
        } else if (strlen(currdir) == 0 && i == 0) {
            /* Root directory */
            strcpy(full_path, "/");
        } else {
            size_t fpl = strlen(full_path);
            if (fpl > 0) {
                if (full_path[fpl - 1] != '/') {
                    strcat(full_path, "/");
                }
                strcat(full_path, currdir);
            }
        }
        /* Set currdir to zero len */
        k = 0;
    }

    klogd("sys_chdir: currdir \"%s\", targetdir \"%s\" and change to \"%s\"",
          pcurr->cwd, dir, full_path);

    if (!vfs_path_to_node(full_path, NO_CREATE, 0)) {
        cpu_set_errno(ENOENT);
        return -1;
    }

    strcpy(pcurr->cwd, full_path);
    return 0;
}