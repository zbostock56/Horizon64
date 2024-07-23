/**
 * @file clkhanlder.c
 * @author Zack Bostock
 * @brief Interrupt handler for the clock tick external interrupt.
 * @verbatim
 * Here, the clock interrupt is serviced. This is where we handle scheduling
 * for the process which will gain control of the CPU. Additionally, we can
 * track uptime for the system (if desired).
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <globals.h>

static volatile uint64_t system_time = 0;
static volatile uint8_t preempt_quantum = QUANTUM;

/**
 * @brief Keeps the tick of the kernel. Handles scheduling and keeping time.
 */
void clkhandler() {
  system_time++;
  if (--preempt_quantum <= 0) {
    preempt_quantum = QUANTUM;
    /* TODO: Implement scheduling */
    /* Reschedule */
  }
}
