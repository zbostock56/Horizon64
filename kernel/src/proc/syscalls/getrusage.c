/**
 * @file getrusage.c
 * @author Zack Bostock
 * @brief Functionality pertaining to getrusage system call
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <proc/syscalls/getrusage.h>
#include <proc/syscall.h>

#include <string.h>
#include <common/kprint.h>

/**
 * @brief Gets system usage information
 *
 * @param who Who to get information about
 * @param usage Buffer to copy usage information into
 * @return int 0 if success, -1 if failure
 */
int64_t sys_getrusage(int64_t who, uint64_t usage) {
    /* TODO: implement */
    RUSAGE *u = (RUSAGE *) usage;

    klogw("sys_getrusage: (not implemented) getting rusage for %x\n", who);
    memset(u, 0, sizeof(RUSAGE));

    return 0;
}
