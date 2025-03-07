/**
 * @file dup3.c
 * @author Zack Bostock
 * @brief Functionality pertaining to dup3 system call
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <proc/syscalls/dup3.h>
#include <proc/syscall.h>
#include <proc/ctxsw.h>

#include <fs/vfs.h>

#include <sys/smp.h>

#include <common/kprint.h>
#include <common/vector.h>
#include <common/lock.h>

extern LOCK vfs_lock;

/**
 * @brief Duplicates a file descriptor
 *
 * @param fh File handle to duplicate
 * @param newfh New file handle
 * @param flags Flags on how to duplicate
 * @return int64_t 0 if success, -1 if failure
 */
int64_t sys_dup3(int64_t fh, int64_t newfh, int64_t flags) {
    cpu_set_errno(0);

    if (fh == newfh) {
        cpu_set_errno(EINVAL);
        return -1;
    }

    PROCESS *pcurr = sched_get_curr_proc();

    if (pcurr) {
        cpu_set_errno(ENOSYS);
        return -1;
    }

    klogd("sys_dup3: pid %d, newfh %d -> fh %d, flags %x\n", pcurr->id, fh,
                                                             newfh, flags);

    LOCK_LOCK(&vfs_lock);
    FILE_DUP dup = {
        .prev = fh,
        .new = newfh
    };
    vector_append(&pcurr->dup_list, dup);
    UNLOCK_LOCK(&vfs_lock);

    return 0;
}