/**
 * @file getppid.c
 * @author Zack Bostock
 * @brief Functionality pertaining to getppid system call
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <proc/syscalls/getppid.h>
#include <proc/syscall.h>

#include <sys/smp.h>

/**
 * @brief Gets the process ID of the parent of the calling process
 *
 * @return int64_t process id of parent of the calling process, or -1 if failure
 */
int64_t sys_getppid() {
    /* TODO: implement */
    cpu_set_errno(ENOSYS);
    return -1;
}
