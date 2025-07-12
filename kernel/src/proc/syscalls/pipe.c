/**
 * @file pipe.c
 * @author Zack Bostock
 * @brief Functionality pertaining to pipe system call
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <proc/syscalls/pipe.h>
#include <proc/syscall.h>
#include <proc/process.h>
#include <proc/ctxsw.h>

#include <fs/vfs.h>

#include <sys/smp.h>

#include <common/kprint.h>

static int pipe_number = 0;

/**
 * @brief Helper to convert an integer to a string
 *
 * @param num Number to convert
 * @param str String representation
 */
static inline void int_to_str(unsigned int num, char *str) {
    if (num == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }

    /* Start from the end of the buffer */
    char *ptr = str;
    while (num > 0) {
        *ptr++ = (num % 10) + '0';
        num /= 10;
    }

    /* Null-terminate and reverse the string in-place */
    *ptr = '\0';
    char *start = str, *end = ptr - 1;
    while (start < end) {
        char temp = *start;
        *start++ = *end;
        *end-- = temp;
    }
}

/**
 * @brief Creates a pipe
 *
 * @param pipefd Front and back file descriptors for a pipe
 * @return int64_t
 */
int64_t sys_pipe(int32_t pipefd[2]) {
    cpu_set_errno(0);
    PROCESS *pcurr = sched_get_curr_proc();

    if (!pcurr) {
        cpu_set_errno(ENODEV);
        return -1;
    } else if (pcurr->id < 1) {
        kloge("sys_pipe: Found invalid process id (%d)\n", pcurr->id);
        cpu_set_errno(ESRCH);
        return -1;
    }

    char path[VFS_MAX_NAME_LEN] = {0};
    strcpy(path, "/dev/pipe/");

    /* Add number on the end to signify which pipe it is */
    char number_to_string[64] = {0};
    int_to_str(++pipe_number, number_to_string);
    strcat(path, number_to_string);

    vfs_create(path, VFS_CHAR_DEV);

    /* [0] is the reading port, [1] is the writing port */
    pipefd[0] = vfs_open(path, VFS_READ);
    pipefd[1] = vfs_open(path, VFS_WRITE);

    klogd("sys_pipe: created new pipe (read %d fd, write %d fd) at %s\n",
          pipefd[0], pipefd[1], path);
    return 0;
}
