/**
 * @file meminfo.c
 * @author Zack Bostock
 * @brief Functionality pertaining to meminfo system call
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <proc/syscalls/meminfo.h>
#include <proc/syscall.h>
#include <proc/process.h>
#include <proc/ctxsw.h>

#include <sys/smp.h>
#include <sys/mmu.h>

#include <common/kprint.h>

/**
 * @brief Prints memory information
 *
 * @return int64_t -1 if failure, 0 if success
 */
int64_t sys_meminfo() {
    cpu_set_errno(0);
    PROCESS *pcurr = sched_get_curr_proc();

    if (!pcurr) {
        cpu_set_errno(ENODEV);
        return -1;
    }

    if (pcurr->id < 1) {
        kloge("sys_meminfo: Found invalid process id (%d)\n", pcurr->id);
        cpu_set_errno(ESRCH);
        return -1;
    }

    pm_used();
    return 0;
}
