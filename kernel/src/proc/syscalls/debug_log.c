/**
 * @file debug_log.c
 * @author Zack Bostock
 * @brief Functionality pertaining to debug_log system call
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <proc/syscalls/debug_log.h>

#include <common/kprint.h>
#include <common/string.h>

/**
 * @brief Allows userspace programs to print debug messages to the screen
 *
 * @param msg Message to print
 * @return int64_t Number of characters printed to the screen, or -1 if failure
 */
int64_t sys_debug_log(char *msg) {
    if (!msg) {
        return -1;
    }
    char *s = strchr(msg, '\n');
    if (s && (*(s + 1) == '\0')) {
        *s = '\0';
    } else {
        return -1;
    }
    klogd("sys_debug_log: %s\n", msg);
    return strlen(msg);
}