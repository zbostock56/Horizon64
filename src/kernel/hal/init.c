#include "init.h"
#include <arch/i686/gdt.h>
#include <arch/i686/idt.h>
#include <arch/i686/isr.h>
#include <arch/i686/irq.h>

/*
    Main function of the "HAL" (hardware abstraction layer).

    Sets up GDT, IDT, ISR and IRQ.
*/
void system_init() {
    i686_gdt_init();
    i686_idt_init();
    i686_isr_init();
    i686_irq_init();
}