#include <pic.h>

void pic_set_mask(uint16_t mask) {
  g_pic_mask = mask;
  outb(PIC1_DATA_PORT, g_pic_mask & 0xFF);
  io_wait();
  outb(PIC2_DATA_PORT, g_pic_mask >> 8);
  io_wait();
}

void pic_disable() {
  pic_set_mask(0xFFFF);
}

void pic_mask(int irq) {
  pic_set_mask(g_pic_mask | (1 << irq));
}

void pic_unmask(int irq) {
  terminal_printf(&term, "Unmasked IRQ %d\n", irq);
  pic_set_mask(g_pic_mask & ~(1 << irq));
}

uint16_t pic_get_mask() {
  return inb(PIC1_DATA_PORT) | (inb(PIC2_DATA_PORT) << 8);
}

uint16_t pic_read_in_request_register() {
  outb(PIC1_COMMAND_PORT, PIC_CMD_READ_IRR);
  outb(PIC2_COMMAND_PORT, PIC_CMD_READ_IRR);
  return ((uint16_t) inb(PIC2_COMMAND_PORT)) |
         (((uint16_t) inb(PIC2_COMMAND_PORT)) << 8);
}

uint16_t pic_read_in_service_register() {
  outb(PIC1_COMMAND_PORT, PIC_CMD_READ_ISR);
  outb(PIC2_COMMAND_PORT, PIC_CMD_READ_ISR);
  return ((uint16_t) inb(PIC2_COMMAND_PORT)) |
         (((uint16_t) inb(PIC2_COMMAND_PORT)) << 8);
}

int pic_probe() {
  pic_disable();
  pic_set_mask(0x0);
  return (pic_get_mask() == 0x0);
}

void pic_send_end_of_interrupt(int irq) {
  if (irq > 8) {
    outb(PIC2_COMMAND_PORT, PIC_CMD_END_OF_INTERRUPT);
  }
  outb(PIC1_COMMAND_PORT, PIC_CMD_END_OF_INTERRUPT);
}

void pic_configure(uint8_t offset_pic_1, uint8_t offset_pic_2) {
  /* Mask all interrupts */
  pic_set_mask(0xFFFF);

  /* Initialize control word 1 */
  outb(PIC1_COMMAND_PORT, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);
  io_wait();
  outb(PIC2_COMMAND_PORT, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);
  io_wait();

  /* Initialize control word 2 - the offsets */
  /* Offsets denote what interrupt number offset */
  /* the PIC will report when an interrupt occurs */
  outb(PIC1_DATA_PORT, offset_pic_1);
  io_wait();
  outb(PIC2_DATA_PORT, offset_pic_2);
  io_wait();

  /* Initialize control word 3 */
  /* Tell PIC1 that the slave PIC is at IRQ2 (b0100 || 0x4) */
  outb(PIC1_DATA_PORT, 0x4);
  io_wait();
  /* Tell PIC2 that its cascade identity is (b0010 || 0x2) */
  outb(PIC2_DATA_PORT, 0x2);
  io_wait();

  /* Initialize control word 4 */
  if (g_auto_eoi) {
    outb(PIC1_DATA_PORT, PIC_ICW4_8086 | PIC_ICW4_AUTO_EOI);
  } else {
    outb(PIC1_DATA_PORT, PIC_ICW4_8086);
  }
  io_wait();
  if (g_auto_eoi) {
    outb(PIC2_DATA_PORT, PIC_ICW4_8086 | PIC_ICW4_AUTO_EOI);
  } else {
    outb(PIC2_DATA_PORT, PIC_ICW4_8086);
  }
  io_wait();

  /* Mask all interrupts until they are enabled after initialization */
  pic_set_mask(0xFFFF);
}

const PIC_DRIVER *pic_get_driver() {
  return &g_pic_driver;
}
