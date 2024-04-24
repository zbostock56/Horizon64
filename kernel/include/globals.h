#pragma once

#include <stdint.h>
#include <stddef.h>
#include <const.h>
#include <asm.h>

/* System time and system timer information */
extern uint64_t system_time;
extern uint8_t preempt_quantum;