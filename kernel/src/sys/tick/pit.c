#include <pit.h>

void pit_init(uint32_t hertz) {
  /* Check to make sure the frequency is within range */
  if (hertz < PIT_MIN_FREQ || hertz > PIT_MAX_FREQ) {
    kprintf("PIT programmed out of range! Setting to default 1 ms...\n");
    hertz = PIT_DEFAULT_FREQ;
  }

  /* Calculate the value to send to the PIT */
  hertz = PIT_MAX_FREQ / hertz;

  /* Inform the PIT what modes we are using */
  uint8_t mode = PIT_BINARY_MODE |
                 PIT_MODE_COMMAND_SET_OP_MODE(PIT_MODE_3) |
                 PIT_MODE_COMMAND_SET_ACCESS_MODE(PIT_LO_AND_HIGH) |
                 PIT_MODE_COMMAND_SET_CHANNEL(PIT_CHANNEL_0);

  /* Program the PIT with the frequency */
  outb(mode, MODE_COMMAND_REGISTER);
  outb(hertz & 0x00FF, PIT_CHANNEL_0_DATA_PORT);           /* LSB */
  outb((hertz & 0xFF00) >> 8, PIT_CHANNEL_0_DATA_PORT);    /* MSB */
  
  /* Set the interrupt handler for the PIT (IRQ 0) to be serviceable */
  pic_unmask(0);

  kprintf("PIT set to %d hz and IRQ0 is unmasked...\n", hertz);
}

void pit_set_event(unsigned long delta) {
  /* Used for timer events */
  disable_interrupts();
  outb(delta & 0xFF, PIT_CHANNEL_0_DATA_PORT);  /* LSB */
  outb(delta >> 8, PIT_CHANNEL_0_DATA_PORT);    /* MSB */
  enable_interrupts();
}

unsigned int pit_read_frequency() {
  unsigned int count = 0;

  disable_interrupts();
  outb(MODE_COMMAND_REGISTER, PIT_LATCH_COUNT_COMMAND);

  count = inb(PIT_CHANNEL_0_DATA_PORT);       /* LSB */
  count |= inb(PIT_CHANNEL_0_DATA_PORT) << 8; /* MSB */

  enable_interrupts();
  return count;
}

const PIT_DRIVER *pit_get_driver() {
  return &g_pit_driver;
}