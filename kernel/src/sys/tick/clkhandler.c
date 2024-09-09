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
#include <structs/regs_str.h>

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

void apic_send_end_of_interrupt();
uint8_t apic_timer_int_is_delivered();

void clkhandler_two(REGISTERS *) {
  system_time++;
  klogi("NEW HANDLER SYSTEM TIME: %d\n", system_time);
  if (--preempt_quantum <= 0) {
    preempt_quantum = QUANTUM;
    /* TODO: Implement scheduling */
    /* Reschedule */
  }
  klogi("before Interrupt delivered: %d\n", apic_timer_int_is_delivered());
  apic_send_end_of_interrupt();
  klogi("after Interrupt delivered: %d\n", apic_timer_int_is_delivered());
}

/**
 * @brief Helper to get the system time variable
 * 
 * @return uint64_t Ticks from system boot
 */
uint64_t get_pit_time() {
    return system_time;
}

/**
 * @brief Helper for sleeping for a specific amount of time based on the
 *        system timer
 * @note This only works if the system timer is set to tick every 1 ms
 * 
 * @param offset Milliseconds to sleep
 */
void system_timer_sleep(uint64_t offset) {
    uint64_t start = system_time;
    while ((start + offset) > system_time) {
        __asm__("nop;");
    }
}