/**
 * @file getpid.c
 * @author Zack Bostock
 * @brief Functionality pertaining to getpid system call
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <proc/syscalls/getpid.h>
#include <proc/syscall.h>
#include <proc/process.h>
#include <proc/ctxsw.h>

#include <sys/cpu.h>
#include <sys/smp.h>

#include <common/kprint.h>

/**
 * @brief System call implemenation for getting the running process' pid
 *
 * @return int64_t -1 if failure, otherwise running process' pid
 */
int64_t sys_getpid() {
    cpu_set_errno(0);
    PROCESS *pcurr = sched_get_curr_proc();
    if (pcurr) {
        klogd("sys_getpid: process %d\n", pcurr->id);
        if (pcurr->id >= 1) {
            return pcurr->id;
        } else {
            kloge("sys_getpid: Found invalid process id: (%d)\n", pcurr->id);
            halt();
        }
    }

    cpu_set_errno(EINVAL);
    return -1;
}