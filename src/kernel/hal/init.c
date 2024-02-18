#include "init.h"
#include "kernel.h"
#include <arch/i686/gdt.h>
#include <arch/i686/idt.h>
#include <arch/i686/isr.h>
#include <arch/i686/irq.h>
#include <arch/i686/pic.h>

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
    i686_gdt_init();
    i686_idt_init();
    i686_isr_init();
    i686_irq_init();
}