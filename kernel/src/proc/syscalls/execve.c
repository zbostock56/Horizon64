/**
 * @file execve.c
 * @author Zack Bostock
 * @brief Functionality pertaining to execve system call
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <proc/syscalls/execve.h>
#include <proc/process.h>
#include <proc/syscall.h>
#include <proc/ctxsw.h>

#include <sys/cpu.h>
#include <sys/smp.h>

/**
 * @brief System call implementation for execve
 *
 * @param path Path to executable binary
 * @param argv Arguments to pass to execution
 * @param envp Environment variables to pass to execution
 * @return int64_t 0 on success, -1 on failure
 */
int64_t sys_execve(const char *path, const char *argv[], const char *envp[]) {
    char *cwd = NULL;
    PROCESS *pcurr = sched_get_curr_proc();
    if (pcurr) {
        cwd = pcurr->cwd;
    }

    if (sched_execve(path, argv, envp, cwd)) {
        sched_exit(0);
        cpu_set_errno(0);
        return 0;
    } else {
        cpu_set_errno(EINVAL);
        return -1;
    }
}