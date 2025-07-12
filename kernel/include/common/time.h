/**
 * @file time.h
 * @author Zack Bostock
 * @brief Information related to time functionality
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>

#include <structs/time_str.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */
#define SECONDS_TO_NANOS(x) ((x) * 1000000000ULL)
#define MILLIS_TO_NANOS(x)  ((x) * 1000000ULL)
#define MICROS_TO_NANOS(x)  ((x) * 1000ULL)
#define NANOS_TO_SECONDS(x) ((x) / 1000000000ULL)
#define NANOS_TO_MILLIS(x)  ((x) / 1000000ULL)
#define NANOS_TO_MICROS(x)  ((x) / 1000ULL)

#define IS_LEAP_YEAR(year) ((year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0)))

/* --------------------------- INTERNALLY DEFINED --------------------------- */
uint64_t sec_in_years(uint64_t years);
uint64_t sec_in_months(uint64_t months, uint64_t year);
int day_month_year_to_year_day(int day, int month, int year);
int days_in_month(int month, int year);
int day_of_week(uint64_t seconds);
void seconds_to_std_time(const uint64_t seconds, STD_TIME *st);