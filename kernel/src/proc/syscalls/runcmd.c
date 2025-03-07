/**
 * @file runcmd.c
 * @author Zack Bostock
 * @brief Functionality pertaining to runcmd system call
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <proc/syscalls/runcmd.h>
#include <proc/syscall.h>
#include <proc/process.h>
#include <proc/ctxsw.h>

#include <sys/pci.h>

#include <sys/smp.h>

#include <common/kprint.h>

/**
 * @brief Runs internal kernel functionality
 *
 * @param cmd Command to run
 * @return int64_t 0 if successful, -1 if failure
 */
int64_t sys_runcmd(char *cmd) {
    if (!strcmp(cmd, "lspci")) {
        pci_list();
        return 0;
    } else {
        cpu_set_errno(EINVAL);
        return -1;
    }
}
