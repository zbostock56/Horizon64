/**
 * @file munmap.c
 * @author Zack Bostock
 * @brief Functionality pertaining to munmap system call
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <proc/syscalls/munmap.h>
#include <proc/syscall.h>
#include <proc/process.h>
#include <proc/ctxsw.h>

#include <sys/mmu.h>
#include <sys/smp.h>

#include <common/kprint.h>

/**
 * @brief Unmap memory
 *
 * @param ptr Pointer to start of memory section
 * @param size Size of memory section
 * @return int64_t 0 if success, -1 if failure
 */
int64_t sys_vm_unmap(void *ptr, size_t size) {
    /* TODO: Need to implement memory free */
    cpu_set_errno(0);

    if (size == 0) {
        cpu_set_errno(EINVAL);
        return (int64_t) NULL;
    }

    PROCESS *pcurr = sched_get_curr_proc();
    ADDR_SPACE *as = NULL;
    if (pcurr) {
        if (pcurr->id < 1) {
            kloge("sys_vm_unmap: Found invalid process id (%d)\n", pcurr->id);
            halt();
        }
        as = pcurr->addrspace;
    }

    uint64_t np = NUM_PAGES(size);
    vm_unmap(as, (uint64_t) ptr, np);

    if (pcurr) {
        klogd("sys_vm_unmap: process %d unmap %x with %d pages\n", pcurr->id,
                                                                   ptr, np);
    }
    return 0;
}
