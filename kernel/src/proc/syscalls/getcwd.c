/**
 * @file getcwd.c
 * @author Zack Bostock
 * @brief Functionality pertaining to getcwd system call
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <proc/syscalls/getcwd.h>
#include <proc/process.h>
#include <proc/ctxsw.h>
#include <proc/syscall.h>

#include <sys/smp.h>

/**
 * @brief Get current working directory
 *
 * @param buffer Buffer to copy current working directory into
 * @param size Size of buffer
 * @return int64_t -1 if failure, 0 if success
 */
int64_t sys_getcwd(char *buffer, size_t size) {
    PROCESS *pcurr = sched_get_curr_proc();
    cpu_set_errno(0);

    if (!buffer || size == 0) {
        cpu_set_errno(EINVAL);
        return -1;
    }

    if (!pcurr) {
        cpu_set_errno(ENODEV);
        return -1;
    }

    if (pcurr->id < 1) {
        cpu_set_errno(ESRCH);
        return -1;
    }

    size_t len = strlen(pcurr->cwd);
    if (len < size - 1) {
        strcpy(buffer, pcurr->cwd);
    } else {
        cpu_set_errno(ENAMETOOLONG);
        return -1;
    }

    return 0;
}