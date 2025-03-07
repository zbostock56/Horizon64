/**
 * @file getclock.h
 * @author Zack Bostock
 * @brief Functionality pertaining to getclock system call
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>
#include <structs/vfs_str.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/**
 * @brief Constants for the clock
 */
#define CLOCK_REALTIME          (0)
#define CLOCK_MONOTONIC         (1)
#define CLOCK_PROC_CPUTIME_ID   (2)
#define CLOCK_THREAD_CPUTIME_ID (3)
#define CLOCK_MONOTONIC_RAW     (4)
#define CLOCK_REALTIME_COARSE   (5)
#define CLOCK_MONOTONIC_COARSE  (6)
#define CLOCK_BOOTTIME          (7)

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
int64_t sys_getclock(void *unused, int64_t which, VFS_TIMESPEC *out);

