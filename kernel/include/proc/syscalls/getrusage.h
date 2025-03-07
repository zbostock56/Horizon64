/**
 * @file getrusage.h
 * @author Zack Bostock
 * @brief Functionality pertaining to getrusage system call
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/**
 * @brief Used in getting system resource usage information
 */
typedef struct {
    uint64_t ru_utime;          /* User CPU time used */
    uint64_t ru_stime;          /* System CPU time used */
    int64_t ru_maxrss;          /* Maximum resident set size */
    int64_t ru_ixrss;           /* Integral shared memory size */
    int64_t ru_idrss;           /* Integral unshared data size */
    int64_t ru_isrss;           /* Integral unshared stack size */
    int64_t ru_minflt;          /* Page reclaims (soft page faults) */
    int64_t ru_nswap;           /* Swaps */
    int64_t ru_inblock;         /* block input operations */
    int64_t ru_oublock;         /* block output operations */
    int64_t ru_msgsnd;          /* IPC messages sent */
    int64_t ru_msgrcv;          /* IPC messages received */
    int64_t ru_nsignals;        /* Signals received*/
    int64_t ru_nvcsw;           /* Volunatry context switches */
    int64_t ru_nivcsw;          /* Involuntary context switches */
} RUSAGE;

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
int64_t sys_getrusage(int64_t who, uint64_t usage);

