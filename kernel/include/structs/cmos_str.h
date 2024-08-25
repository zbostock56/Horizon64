/**
 * @file cmos_str.h
 * @author Zack Bostock
 * @brief Structures pertaining to CMOS and RTC (Real-Time Clock)
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <stdint.h>

/**
 * @brief CMOS structure from OSDev Wiki
 * @ref https://wiki.osdev.org/CMOS
 */
typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t weekdays;
    uint8_t day;
    uint8_t month;
    uint16_t year;
    uint8_t century;
} CMOS;