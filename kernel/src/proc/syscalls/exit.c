/**
 * @file exit.c
 * @author Zack Bostock
 * @brief Functionality pertaining to exit system call
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <proc/syscalls/exit.h>
#include <proc/process.h>
#include <proc/ctxsw.h>

#include <common/kprint.h>

/**
 * @brief Exits a process
 *
 * @param status Status to exit with
 * @return int64_t Should never return
 */
void sys_exit(int64_t status) {
    PROCESS *pcurr = sched_get_curr_proc();
    if (pcurr) {
        klogd("sys_exit: process %d (%s) exit with status %d\n", pcurr->id,
                                                                 pcurr->name,
                                                                 status);
    }
    sched_exit(status);
}