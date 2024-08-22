/**
 * @file serial.h
 * @author Zack Bostock
 * @brief Information pertaining to serial port driver
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <globals.h>

#include <sys/asm.h>

#include <common/string.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
/* Addresses for the COM ports. The first two are fairly guaranteed whereas */
/* the rest are more BIOS specific. */
#define COM1      (0x3F8)
#define COM2      (0x2F8)
#define COM3      (0x3E8)
#define COM4      (0x2E8)
#define COM5      (0x5F8)
#define COM6      (0x4F8)
#define COM7      (0x5E8)
#define COM8      (0x4E8)

#define BAUD_RATE (38400)
#define MAX_RATE  (115200)

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
STATUS serial_init();
uint8_t serial_recieved();
uint8_t is_transmit_empty();
STATUS serial_write(char c);
char serial_read();
STATUS serial_puts(const char *str);
