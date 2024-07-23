/**
 * @file pic.c
 * @author Zack Bostock
 * @brief External interrupt initialization
 * @verbatim
 * Here we initialize the Programmable Interrupt Controller (PIC), particularly
 * the Intel 8259 chip. When external interrupts occur, the PIC is notified on
 * one of its interrupt lines. The PIC is then responsible for notifying the
 * processor which interrupt has occured, assuming external interrupts are
 * enabled. After initialization, all interrupts are masked and the device
 * drivers individually unmask the interrupt lines which correspond to their
 * devices.
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <sys/tick/pic.h>

static uint16_t g_pic_mask = 0xFFFF;
static int g_auto_eoi = 0;

enum {
    PIC_ICW1_ICW4           = 0x01,     /* If set, ICW4 must be read (its optional) */
    PIC_ICW1_SINGLE         = 0x02,     /* If set, means there is only one 8259 PIC */
    PIC_ICW1_INTERVAL4      = 0x04,     /* Address interval */
    PIC_ICW1_LEVEL          = 0x08,     /* Edge or level triggered mode */
    PIC_ICW1_INITIALIZE     = 0x10,     /* Tell PIC we are initializing (required!) */
} PIC_ICW1;

enum {
    PIC_ICW4_8086           = 0x01,      /* Set for 8086 platform */
    PIC_ICW4_AUTO_EOI       = 0x02,      /* Automatically set the end of interrupt flag */
    PIC_ICW4_BUFFER_MASTER  = 0x04,      /* Do we buffer the master PIC? */
    PIC_ICW4_BUFFER_SLAVE   = 0x00,      /* Do we buffer the slave PIC? */
    PIC_ICW4_BUFFERRED      = 0x08,      /* Do we buffer interrupts as they come in? */
    PIC_ICW4_SFNM           = 0x10,      /* Specially fully nested mode */
} PIC_ICW4;

enum {
    PIC_CMD_END_OF_INTERRUPT    = 0x20,
    PIC_CMD_READ_IRR            = 0x0A,
    PIC_CMD_READ_ISR            = 0x0B,
} PIC_CMD;


/**
 * @brief Sets the mask of the PIC.
 *
 * @param mask Specified mask to set
 */
void pic_set_mask(uint16_t mask) {
  g_pic_mask = mask;
  outb(PIC1_DATA_PORT, g_pic_mask & 0xFF);
  io_wait();
  outb(PIC2_DATA_PORT, g_pic_mask >> 8);
  io_wait();
}

/**
 * @brief Disables PIC-based interrups by masking all interrupt lines.
 */
void pic_disable() {
  pic_set_mask(0xFFFF);
}

/**
 * @brief Masks an interrupt line.
 *
 * @param irq Interrupt line number to mask
 */
void pic_mask(int irq) {
  pic_set_mask(g_pic_mask | (1 << irq));
}

/**
 * @brief Unmasks an interrupt line.
 *
 * @param irq Interrupt line to unmask
 */
void pic_unmask(int irq) {
  klogi("Unmasked IRQ %d\n", irq);
  pic_set_mask(g_pic_mask & ~(1 << irq));
}

/**
 * @brief Fetches the mask for which interrupts are masked.
 *
 * @return uint16_t Retreived mask
 */
uint16_t pic_get_mask() {
  return inb(PIC1_DATA_PORT) | (inb(PIC2_DATA_PORT) << 8);
}

/**
 * @brief Reads the in-request register on the PIC.
 *
 * @return uint16_t Contents of the in-request register
 */
uint16_t pic_read_in_request_register() {
  outb(PIC1_COMMAND_PORT, PIC_CMD_READ_IRR);
  outb(PIC2_COMMAND_PORT, PIC_CMD_READ_IRR);
  return ((uint16_t) inb(PIC1_COMMAND_PORT)) |
         (((uint16_t) inb(PIC2_COMMAND_PORT)) << 8);
}

/**
 * @brief Reads the in-service register on the PIC.
 *
 * @return uint16_t Contents of the in-service register
 */
uint16_t pic_read_in_service_register() {
  outb(PIC1_COMMAND_PORT, PIC_CMD_READ_ISR);
  outb(PIC2_COMMAND_PORT, PIC_CMD_READ_ISR);
  return ((uint16_t) inb(PIC1_COMMAND_PORT)) |
         (((uint16_t) inb(PIC2_COMMAND_PORT)) << 8);
}

/**
 * @brief Establishes existance of the PIC.
 *
 * @return int Returns 1 if exists, 0 otherwise
 */
int pic_probe() {
  pic_disable();
  pic_set_mask(0x0);
  return (pic_get_mask() == 0x0);
}

/**
 * @brief Send the end of interrupt.
 *
 * @param irq Interrupt line to send the end of interrupt on
 */
void pic_send_end_of_interrupt(int irq) {
  if (irq > 8) {
    outb(PIC2_COMMAND_PORT, PIC_CMD_END_OF_INTERRUPT);
  }
  outb(PIC1_COMMAND_PORT, PIC_CMD_END_OF_INTERRUPT);
}

/**
 * @brief Main initialization function for the PIC.
 *
 * @param offset_pic_1 Offset for the master controller
 * @param offset_pic_2 Offset for the slave controller
 */
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

/**
 * @brief Gets the PIC driver structure for access to driver functions.
 *
 * @return const PIC_DRIVER* PIC driver
 */
const PIC_DRIVER *pic_get_driver() {
  return &g_pic_driver;
}
