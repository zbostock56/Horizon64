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

#include <sys/asm.h>
#include <sys/tick/pic.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
/* Max and min PIT programmable frequencies */
#define PIT_MIN_FREQ     (18)
#define PIT_MAX_FREQ     (1193182)
#define PIT_DEFAULT_FREQ (1000)

/* Read/write channels */
#define PIT_CHANNEL_0_DATA_PORT         (0x40)
#define PIT_CHANNEL_1_DATA_PORT         (0x41)
#define PIT_CHANNEL_2_DATA_PORT         (0x42)

/* Write only channel */
#define MODE_COMMAND_REGISTER           (0x43)

/* -------------------------------- GLOBALS --------------------------------- */

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

/* --------------------------------- DRIVER --------------------------------- */
static const PIT_DRIVER g_pit_driver = {
    .name = "8253 PIT",
    .initialize = &pit_init,
    .set_event = &pit_set_event,
    .read_frequency = &pit_read_frequency,
};
