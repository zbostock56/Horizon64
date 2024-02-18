#pragma once

/*

    KERNEL.H

    Houses globals that are used throughout the kernel and operating system
    for various operations.

*/

#include <stdint.h>

/* Default for scheduling quantum time */
#define QUANTUM (5)

/* System time and system timer information */
extern uint32_t system_time;
extern uint8_t preempt_quantum;