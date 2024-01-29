#include <stddef.h>
#include "irq.h"
#include "pic.h"
#include "general.h"
#include "stdio.h"

#define PIC_REMAP_OFFSET     (0x20)

irq_handler g_irq_handlers[16];
static const PICDRIVER *g_driver = NULL;

void i686_irq_handler(REGISTERS *regs) {
    int irq = regs->interrupt - PIC_REMAP_OFFSET;

    if (g_irq_handlers[irq]) {
        /* Handle IRQ */
        g_irq_handlers[irq](regs);
    } else {
        printf("WARNING: Unhandled IRQ %d...\n", irq);
    }

    /* SEND EOI */
    g_driver->send_end_of_interrupt(irq);
}

void i686_irq_init() {
    const PICDRIVER *drivers[] = {
        i8259_get_driver(),
    };

    for (int i = 0; i < (sizeof(drivers) / sizeof(drivers[0])); i++) {
        if (drivers[i]->probe()) {
            g_driver = drivers[i];
        }
    }

    if (g_driver == NULL) {
        printf("WARNING: No PIC found!\n");
        return;
    }

    printf("Found %s PIC\n", g_driver->name);
    g_driver->initialize(PIC_REMAP_OFFSET, PIC_REMAP_OFFSET + 8);

    /* Register ISR handlers for each of the 16 IRQ lines */
    for (int i = 0; i < 16; i++) {
        i686_isr_register_handler(PIC_REMAP_OFFSET + i, i686_irq_handler);
    }

    /* Enable interrupts */
    i686_enable_interrupts();
}

void i686_irq_register_handler(int irq, irq_handler handler) {
    g_irq_handlers[irq] = handler;
}