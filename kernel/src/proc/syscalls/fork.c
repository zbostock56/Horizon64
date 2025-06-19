/**
 * @file fork.c
 * @author Zack Bostock
 * @brief Functionality pertaining to fork system call
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <proc/syscalls/fork.h>
#include <proc/process.h>
#include <proc/syscall.h>
#include <proc/ctxsw.h>

#include <sys/cpu.h>
#include <sys/smp.h>

#include <common/kprint.h>

/**
 * @brief System call implementation of fork
 *
 * @return int64_t On success, child process returns 0 and pid of child process
 * returns in the parent process; -1 on failure
 */
void print_process_table();
int64_t sys_fork() {
    PROCESS *p = sched_get_curr_proc();
    cpu_set_errno(0);

    if (!p) {
        cpu_set_errno(ENODEV);
        return -1;
    } else if (p->id < 1) {
        kloge("sys_fork: Found invalid process id (%d)\n", p->id);
        cpu_set_errno(ESRCH);
        return -1;
    }

    PROC_ID pchild_id = sched_fork();
    PROC_ID curr_pid = sched_get_pid();
    klogi("sys_fork: Parent id (%d), pcurr (%d), return val (%d)\n",
          p->id, curr_pid, pchild_id);

    print_process_table();

    if (pchild_id == process_get_max_processes()) {
        cpu_set_errno(ECHILD);
        return -1;
    } else if (p->id == curr_pid) {
        /*
            This should be the parent process and returning the child pid, but
            current it returns the parent process id
        */
        klogi("sys_fork: returning %d from parent process (%d)\n", pchild_id, p->id);
        return pchild_id;
    } else {
        /* This *should be* the child process and returning 0 */
        klogi("sys_fork: returning 0 from child process (%d)\n", pchild_id);
        return 0;
    }
    return -1;
}