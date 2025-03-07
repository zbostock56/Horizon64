/**
 * @file waitpid.c
 * @author Zack Bostock
 * @brief Functionality pertaining to waitpid system call
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <proc/syscalls/waitpid.h>
#include <proc/process.h>
#include <proc/syscall.h>
#include <proc/ctxsw.h>

#include <sys/smp.h>

#include <common/kprint.h>

/**
 * @brief Waits on another process to finish
 *
 * @param pid PID of process to wait on (-1 for caller process)
 * @param status Used to tell the status of the calling process
 * @param flags Unused currently
 * @return int64_t 0 if success, -1 if failure
 */
int64_t sys_waitpid(int64_t pid, int32_t *status, int32_t flags) {
    (void) flags;
    cpu_set_errno(0);
    PROCESS *pcurr = sched_get_curr_proc();
    if (status) {
        *status = 0;
    }

    if (pid == -1 && pcurr) {
        uint8_t all_dead = TRUE;
        for (size_t i = 0; i < vector_len(&(pcurr->child_list)); i++) {
            PROC_ID pchild_id = vector_at(&(pcurr->child_list), i);
            PROC_STATE pchild_state = sched_get_proc_state(pchild_id);
            if (pchild_state == PROC_UNKNOWN) {
                klogv("sys_waitpid: pid %d -> child pid %d status: ACTIVE\n",
                    pcurr->id, pchild_id);
                all_dead = FALSE;
            } else if (pchild_state == PROC_DEAD) {
                klogw("sys_waitpid: pid %d -> child pid %d status: DEAD\n",
                    pcurr->id, pchild_id);
            }
        }

        sched_sleep(100);

        if (!all_dead) {
            klogv("sys_waitpid: pid %d has active children\n", pcurr->id);
            return 0;
        } else {
            klogd("sys_waitpid: pid %d has no children\n", pcurr->id);
            cpu_set_errno(ECHILD);
            return -1;
        }
    } else if (pcurr->id == (PROC_ID) pid) {
        klogd("sys_waitpid: pid %d (current process) waits on itself\n", pcurr->id);
        size_t retry_time = 0;
        while (TRUE) {
            uint8_t all_dead = TRUE;
            for (size_t i = 0; i < vector_len(&(pcurr->child_list)); i++) {
                PROC_ID pchild_id = vector_at(&(pcurr->child_list), i);
                PROC_STATE pchild_state = sched_get_proc_state(pchild_id);
                if (pchild_state != PROC_UNKNOWN &&
                    pchild_state != PROC_DEAD &&
                    pchild_state != PROC_DYING) {
                    all_dead = FALSE;
                    break;
                }
            }

            if (!all_dead) {
                sched_sleep(100);
                retry_time++;
                if (retry_time >= 100) {
                    cpu_set_errno(ECHILD);
                    return -1;
                }
            } else {
                return 0;
            }
        }
    } else {
        /* Retry 20 times */
        for (size_t i = 1;; i++) {
            PROC_STATE state = sched_get_proc_state(pid);
            if (state != PROC_DEAD && state != PROC_UNKNOWN) {
                sched_sleep(100);
                if (i == 20) {
                    kloge("sys_waitpid: waitin on pid %d (it is still active)\n", pid);
                    cpu_set_errno(EBUSY);
                    return -1;
                }
            }
        }
        klogd("sys_waitpid: waiting on pid %d which is not active, returning...\n", pid);
        return 0;
    }
}
