/**
 * @file pit.c
 * @author Zack Bostock
 * @brief Initialization for the Programmable Interrupt Timer
 * @verbatim
 * In this file, all the code needed to set up the Programmable Interrupt
 * Timer (PIT), particularly the Intel 8253/8254 chip, is housed. This acts as the
 * main driver code needed for chip functionality beyond initialization as well.
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <sys/tick/pit.h>

/*
    Select which channel is to be configured. Must be valid
    on every write of the mode/command register, regardless
    of te other bits or type of operation being performed.
*/
enum {
    PIT_CHANNEL_0           = 0x0,
    PIT_CHANNEL_1           = 0x1,
    PIT_CHANNEL_2           = 0x2,
    PIT_READ_BACK_COMMAND   = 0x3,      /* 8254 only */
} PIT_MODE_COMMAND_CHANNEL;

/*
    Select which access mode to use. Must be valid on every
    write of the mode/command register. Because data port
    is an 8 bit I/O port and all the values involved are
    16 bit, the PIT chip needs to know what byte each read
    or write to the data port wants. For "lobyte", ony the
    lowest 8 bits of the counter value is read or written to
    or from the data port. For "hibyte", it is the opposite.
    For "lo/hibyte", 16 bits are transferred as a pair with
    the lowest followed by the highest in sequential order
    on the same I/O port.
*/
enum {
    PIT_LATCH_COUNT_COMMAND = 0x0,
    PIT_LO_BYTE_ONLY        = 0x1,
    PIT_HIGH_BYTE_ONLY      = 0x2,
    PIT_LO_AND_HIGH         = 0x3,
} PIT_MODE_COMMAND_ACCESS_MODE;

/*
    Selects which operation mode to set PIT to.
*/
enum {
    PIT_MODE_0              = 0x0,      /* Interrupt on terminal count*/
    PIT_MODE_1              = 0x1,      /* Hardware re-triggerable one-shot */
    PIT_MODE_2              = 0x2,      /* Rate generator */
    PIT_MODE_3              = 0x3,      /* Square wave generator */
    PIT_MODE_4              = 0x4,      /* Software triggered strobe */
    PIT_MODE_5              = 0x5,      /* Hardware triggered strobe */
    PIT_MODE_2_SECOND       = 0x6,      /* Rate generator, same as 0x2 */
    PIT_MODE_3_SECOND       = 0x7,      /* Square wave generator, same as 0x3 */
} PIT_MODE_COMMAND_OPERATING_MODE;

/*
    BCD/Binary bit determines if the PIT will operate in
    binary mode or in BCD mode (where each 4 bits of the
    counter represent a decimal digit, values from 0000 to
    9999). x86 machines only use binary mode.
*/
enum {
    PIT_BINARY_MODE         = 0x0,
    PIT_BCD_MODE            = 0x1,
} PIT_MODE_COMMAND_NUMBER_BASE;


/**
 * @brief Main initialization function for the PIT.
 *
 * @param hertz Frequency in hertz to set the PIT to
 */
void pit_init(uint32_t hertz) {
  /* Check to make sure the frequency is within range */
  if (hertz < PIT_MIN_FREQ || hertz > PIT_MAX_FREQ) {
    kloge("PIT programmed out of range! Setting to default 1 ms...\n");
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

  klogi("PIT set to %d hz...\n", hertz);
}

/**
 * @brief Helper for setting events on the PIT.
 *
 * @param delta Time away from now (current time) for the event
 */
void pit_set_event(unsigned long delta) {
  /* Used for timer events */
  disable_interrupts();
  outb(delta & 0xFF, PIT_CHANNEL_0_DATA_PORT);  /* LSB */
  outb(delta >> 8, PIT_CHANNEL_0_DATA_PORT);    /* MSB */
  enable_interrupts();
}

/**
 * @brief Reads the set frequency on the PIT.
 *
 * @return unsigned int Set frequency on the PIT
 */
unsigned int pit_read_frequency() {
  unsigned int count = 0;

  disable_interrupts();
  outb(MODE_COMMAND_REGISTER, PIT_LATCH_COUNT_COMMAND);

  count = inb(PIT_CHANNEL_0_DATA_PORT);       /* LSB */
  count |= inb(PIT_CHANNEL_0_DATA_PORT) << 8; /* MSB */

  enable_interrupts();
  return count;
}

/**
 * @brief Getter for the PIT driver and its functionality.
 *
 * @return const PIT_DRIVER* PIT driver
 */
const PIT_DRIVER *pit_get_driver() {
  return &g_pit_driver;
}
