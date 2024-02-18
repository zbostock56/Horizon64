#include <stddef.h>
#include "irq.h"
#include "pic.h"
#include "pit.h"
#include "general.h"
#include "stdio.h"

#define PIC_REMAP_OFFSET     (0x20)
#define PIT_1MS              (1000)

static const PIC_DRIVER *pic = NULL;
static const PIT_DRIVER *pit = NULL;

IRQ_HANDLER g_irq_handlers[16];
void clkhandler(REGISTERS *);

void i686_irq_handler(REGISTERS *regs) {
    int irq = regs->interrupt - PIC_REMAP_OFFSET;

    if (g_irq_handlers[irq]) {
        /* Handle IRQ */
        g_irq_handlers[irq](regs);
    } else {
        printf("WARNING: Unhandled IRQ %d...\n", irq);
    }

    /* SEND EOI */
    pic->send_end_of_interrupt(irq);
}

void i686_irq_init() {
    pic = i8259_get_driver();
    pit = i8253_get_driver();

    if (!pic->probe()) {
        printf("WARNING: No PIC found!\n");
        return;
    }

    printf("Found %s...\n", pic->name);
    printf("Found %s...\n", pit->name);

    /* Set the offset to (0x20) for the first interrupt which */
    /* the PIC will notify the CPU on */
    pic->initialize(PIC_REMAP_OFFSET, PIC_REMAP_OFFSET + 8);

    printf("PIC Master offset: %x (%d)...\nPIC Slave offset %x (%d)...\n",
            PIC_REMAP_OFFSET, PIC_REMAP_OFFSET, PIC_REMAP_OFFSET + 8, PIC_REMAP_OFFSET + 8);

    /* Set the programmable interrupt timer */
    pit->initialize(PIT_1MS);
    /* Initializd it's handler */
    i686_irq_register_handler(0, (IRQ_HANDLER)clkhandler);

    /* Register ISR handlers for each of the 16 IRQ lines */
    for (int i = 0; i < 16; i++) {
        i686_isr_register_handler(PIC_REMAP_OFFSET + i, i686_irq_handler);
    }

    /* Enable interrupts */
    i686_enable_interrupts();
}

void i686_irq_register_handler(int irq, IRQ_HANDLER handler) {
    g_irq_handlers[irq] = handler;
}