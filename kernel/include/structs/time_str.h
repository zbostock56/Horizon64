/**
 * @file time_str.h
 * @author Zack Bostock
 * @brief Struct which defines the standard way of keeping time
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

typedef struct {
    int minutes_west;   /* Minutes west of Greenwich */
    int dst_time;       /* Type of DST correction */
} TIMEZONE;

typedef struct {
    int seconds;        /* Seconds (0-59) */
    int minutes;        /* Minutes (0-59) */
    int hours;          /* Hours (0-23) */
    int dom;            /* day of month (1-31) */
    int month;          /* Month (0-11) */
    int year;           /* Year */
    int dow;            /* Day of week (0-6, Sunday = 0) */
    int doy;            /* day of year (0-365, Jan 1 = 0) */
    int isdst;          /* Is daylight savings time? */
} STD_TIME;