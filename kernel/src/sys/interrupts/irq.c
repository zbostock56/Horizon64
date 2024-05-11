#include <irq.h>

void irq_handler(REGISTERS *regs) {
  /* Translate between the vector number to hardware interrupt number */
  int irq = regs->interrupt - PIC_REMAP_OFFSET;

  if (g_irq_handler[irq]) {
    /* Handle hardware interrupt */
    g_irq_handler[irq]();
  } else {
    terminal_printf(&term, "Unhandled hardware interrupt (IRQ) %d...\n", irq);
  }

  /* Send end of interrupt to PIC */
  pic->send_end_of_interrupt(irq);
}

void irq_register_handler(int irq, IRQ_HANDLER handler) {
  terminal_printf(&term, "Registered IRQ handler %d\n", irq);
  g_irq_handler[irq] = handler;
}

void irq_init() {
  disable_interrupts();

  pic = pic_get_driver();
  pit = pit_get_driver();

  /* Check to make sure the PIC exists*/

  if (!pic->probe()) {
    terminal_printf(&term, "WARNING: No PIC found!\n");
    return;
  }

  terminal_printf(&term, "Found %s...\n", pic->name);
  terminal_printf(&term, "Found %s...\n", pit->name);

  /* Set the offset to 0x20 (32) for the first interrupt which the PIC */
  /* will notify the CPU on */

  pic->initialize(PIC_REMAP_OFFSET, PIC_REMAP_OFFSET + 8);

  terminal_printf(&term, "PIC master offset: %x (%d)...\nPIC slave offset: %x (%d)...\n",
          PIC_REMAP_OFFSET, PIC_REMAP_OFFSET, PIC_REMAP_OFFSET + 8,
          PIC_REMAP_OFFSET + 8);

  /* Set the programmable interrupt timer */
  pit->initialize(PIT_1MS);

  /* Initialize it's hardware interrupt handler */
  irq_register_handler(0, (IRQ_HANDLER) clkhandler);

  /* Register the interrupt service routines for each of the 16 IRQs */
  for (int i = 0; i < NUM_HARDWARE_INTERRUPTS; i++) {
    isr_register_handler(PIC_REMAP_OFFSET + i, irq_handler);
  }

  enable_interrupts();
}
