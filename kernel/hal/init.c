#include "init.h"
#include "kernel.h"

/*
    Main function of the "HAL" (hardware abstraction layer).

    Sets up GDT, IDT, ISR, IRQ, and other important system
    variables.
*/

uint32_t system_time;
uint8_t preempt_quantum;

void system_init() {
    
    /* Initialize global system variables */
    system_time = 0;
    preempt_quantum = QUANTUM;

    /* Set up hardware functionality */
}