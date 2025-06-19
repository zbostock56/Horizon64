/**
 * @file set_fs_base.c
 * @author Zack Bostock
 * @brief Functionality pertaining to set_fs_base system call
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <proc/syscalls/set_fs_base.h>
#include <proc/process.h>
#include <proc/ctxsw.h>

#include <common/kprint.h>

#include <sys/cpu.h>

/**
 * @brief System call implemenation for setting fs base
 *
 * @param val Value to set fs base to
 * @return int64_t Returns 0 everything, just for compatibility
 */
int64_t sys_set_fs_base(uint64_t val) {
    PROCESS *pcurr = sched_get_curr_proc();
    if (!pcurr) {
        kloge("%s: Failed to get current process!\n", __func__);
        halt();
    }
    klogd("sys_set_fs_base: process %d set to %x\n", pcurr == NULL ? 0 : pcurr->id,
                                                     val);
    write_msr(MSR_FS_BASE_ADDR, val);
    if (pcurr) {
        pcurr->fs_base = val;
    }

    return 0;
}
