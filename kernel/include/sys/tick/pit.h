/**
 * @file pit.h
 * @author Zack Bostock 
 * @brief Information pertaining to Programmable Interrupt Timer 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <globals.h>
#include <structs/pit_str.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
/* Max and min PIT programmable frequencies */
#define PIT_MIN_FREQ     (18)
#define PIT_MAX_FREQ     (1193180)
#define PIT_DEFAULT_FREQ (1000)

/* Read/write channels */
#define PIT_CHANNEL_0_DATA_PORT         (0x40)
#define PIT_CHANNEL_1_DATA_PORT         (0x41)
#define PIT_CHANNEL_2_DATA_PORT         (0x42)

/* Write only channel */
#define MODE_COMMAND_REGISTER           (0x43)

/* ----------------------------- STATIC GLOBALS ----------------------------- */
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

/* --------------------------------- MACROS --------------------------------- */
#define PIT_MODE_COMMAND_SET_CHANNEL(x)     (x << 6)
#define PIT_MODE_COMMAND_SET_ACCESS_MODE(x) (x << 4)
#define PIT_MODE_COMMAND_SET_OP_MODE(x)     (x << 1)

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void pit_init(uint32_t hertz);
void pit_set_event(unsigned long delta);
unsigned int pit_read_frequency();
const PIT_DRIVER *pit_get_driver();
unsigned int pit_read_frequency();

/* --------------------------- EXTERNALLY DEFINED --------------------------- */
void pic_unmask(int irq);

/* --------------------------------- DRIVER --------------------------------- */
static const PIT_DRIVER g_pit_driver = {
    .name = "8253 PIT",
    .initialize = &pit_init,
    .set_event = &pit_set_event,
    .read_frequency = &pit_read_frequency,
};