/**
 * @file cmos.h
 * @author Zack Bostock
 * @brief Information pertaining to CMOS and RTC (Real-Time Clock) 
 * @verbatim 
 * "CMOS" is a tiny bit of very low power static memory that lives on the
 * same chip as the Real-Time Clock (RTC). It was introduced to IBM PC AT in
 * 1984 which used Motorola MC146818A RTC.
 * @ref https://wiki.osdev.org/CMOS 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <structs/cmos_str.h>

#include <common/memory.h>
#include <common/kprint.h>

#include <sys/asm.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
/**
 * @brief Registers to read in order to find information pertaining to
 * the CMOS structure
 */
#define CMOS_REG_SEC        (0x00)
#define CMOS_REG_MIN        (0x02)
#define CMOS_REG_HRS        (0x04)
#define CMOS_REG_WKD        (0x06)
#define CMOS_REG_DAY        (0x07)
#define CMOS_REG_MTH        (0x08)
#define CMOS_REG_YEAR       (0x09)
#define CMOS_REG_CEN        (0x32)
#define CMOS_REG_STA        (0x0A)
#define CMOS_REG_STB        (0x0B)

/**
 * @brief I/O ports associated with CMOS
 */
#define CMOS_CMD_PORT       (0x70)
#define CMOS_DATA_PORT      (0x71)

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/**
 * @brief Helper macro to get all the data from the registers
 */
#define GET_TIME(rtc)                                                       \
   rtc.seconds = get_reg_value(CMOS_REG_SEC);                               \
   rtc.minutes = get_reg_value(CMOS_REG_MIN);                               \
   rtc.hours = get_reg_value(CMOS_REG_HRS);                                 \
   rtc.weekdays = get_reg_value(CMOS_REG_WKD);                              \
   rtc.day = get_reg_value(CMOS_REG_DAY);                                   \
   rtc.month = get_reg_value(CMOS_REG_MTH);                                 \
   rtc.year = get_reg_value(CMOS_REG_YEAR);                                 \
   rtc.century = get_reg_value(CMOS_REG_CEN);

/**
 * @brief Helper macro to see if two time structures are the same
 */
#define CHECK_TIME_EQ(one, two)                                             \
    (one.seconds == two.seconds)        &&                                  \
    (one.minutes == two.minutes)        &&                                  \
    (one.hours == two.hours)            &&                                  \
    (one.weekdays == two.weekdays)      &&                                  \
    (one.day == two.day)                &&                                  \
    (one.month == two.month)            &&                                  \
    (one.year == two.year)              &&                                  \
    (one.century == two.century)

/**
 * @brief Helper macro to convert BCD to binary
 */
#define TO_BINARY(val) ((val & 0xF) + ((val / 16) * 10)) 

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void cmos_init();