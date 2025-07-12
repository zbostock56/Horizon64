/**
 * @file fnctl.c
 * @author Zack Bostock
 * @brief Functionality pertaining to fnctl system call
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <proc/syscalls/fcntl.h>
#include <proc/syscall.h>

#include <sys/smp.h>

#include <common/kprint.h>

/**
 * @brief Manipulate a file descriptor
 *
 * @param fd File descriptor to manipulate
 * @param request Request on what to do to file descriptor
 * @param args Arguments with the request
 * @return int64_t 0 if success, -1 if failure
 */
int64_t sys_fcntl(int64_t fd, int64_t request, int64_t args) {
    /* TODO: implement */
    klogd("sys_fnctl: fd %d, request %x, arg %x\n", fd, request, args);
    cpu_set_errno(ENOSYS);
    return -1;
}