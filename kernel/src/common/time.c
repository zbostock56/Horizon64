/**
 * @file time.c
 * @author Zack Bostock
 * @brief Implemenation of common time functions
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <common/time.h>
#include <common/kprint.h>

/**
 * @brief Helper to roughly compute number of seconds based on current year
 *
 * @param years Year to convert into seconds
 * @return uint64_t Number of seconds
 */
uint64_t sec_in_years(uint64_t years) {
    uint64_t days = 0;

    while (years > 1969) {
        days += 365;
        if (years % 4 == 0) {
            if (years % 100 == 0) {
                if (years % 400 == 0) {
                    days++;
                }
            } else {
                days++;
            }
        }
        years--;
    }

    return days * 86400;
}

/**
 * @brief Computes the number of seconds based on the month
 *
 * @param months Month to compue from
 * @param year Year to compute from
 * @return uint64_t Number of seconds
 */
uint64_t sec_in_months(uint64_t months, uint64_t year) {
    uint64_t days = 0;

    for (uint64_t i = 1; i <= months; i++) {
        switch (i) {
            case 12:
            case 10:
            case 8:
            case 7:
            case 5:
            case 3:
            case 1:
                days += 31;
                break;
            case 11:
            case 9:
            case 6:
            case 4:
                days += 30;
                break;
            case 2:
                days += 28;
                if ((year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0))) {
                    days++;
                }
            default:
                break;
        }
    }

    return days * 86400;
}

/**
 * @brief Conversion helper from day, month, year into day out of the year
 *
 * @param day Day to convert from
 * @param month Month to convert from
 * @param year Year to convert from
 * @return int Current day number of the year
 */
int day_month_year_to_year_day(int day, int month, int year) {
    int year_day = 0;
    /* Days in month*/
    int dim[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    /* Adjust February for leap years */
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        dim[1] = 29;
    }

    /* Sum the days of the months prior to the current month */
    for (int i = 0; i < month - 1; i++) {
        year_day += dim[i];
    }

    year_day += day;

    return year_day;
}


/**
 * @brief Helper for converting month/year into days in the month
 *
 * @param month Month to convert
 * @param year Year to convert
 * @return int Number of days in the month
 */
int days_in_month(int month, int year) {
    switch(month) {
        case 12:
            return 31;
        case 11:
            return 30;
        case 10:
            return 31;
        case 9:
            return 30;
        case 8:
            return 31;
        case 7:
            return 31;
        case 6:
            return 30;
        case 5:
            return 31;
        case 4:
            return 30;
        case 3:
            return 31;
        case 2:
            return IS_LEAP_YEAR(year) ? 29 : 28;
        case 1:
            return 31;
        default:
            return 0;
    }
    return 0;
}

/**
 * @brief Converts from seconds into day of week
 *
 * @param seconds Current seconds since last epoch
 * @return int Day of the week
 */
int day_of_week(uint64_t seconds) {
    return (((seconds / 86400) + 4) % 7);
}

/**
 * @brief Create a STD_TIME structure from seconds
 *
 * @param seconds Seconds to convert
 * @param st Input STD_TIME structure to populate in function
 */
void seconds_to_std_time(const uint64_t seconds, STD_TIME *st) {
    if (!st) {
        kloge("Trying to convert seconds into NULL STD_TIME input!\n");
        return;
    }
    uint64_t sec = 0;
    uint64_t year_sec = 0;
    for (int year = 1970; year < 2100; year++) {
        uint64_t adder = (IS_LEAP_YEAR(year) ? 366 : 365) * 86400;

        if (sec + adder > seconds) {
            st->year = year - 1900;
            year_sec = sec;
            for (int month = 1; month <= 12; month++) {
                adder = days_in_month(month, year) * 86400;
                if (sec + adder > seconds) {
                    st->month = month - 1;
                    for (int day = 1; day < days_in_month(month, year); day++) {
                        adder = 60 * 60 * 24;
                        if (sec + adder > seconds) {
                            st->dom = day;
                            for (int hour = 1; hour <= 24; hour++) {
                                adder = 60 * 60;
                                if (sec + adder > seconds) {
                                    long remaining = seconds - sec;
                                    st->hours = hour - 1;
                                    st->minutes = remaining / 60;
                                    st->seconds = remaining % 60;
                                    st->dow = day_of_week(seconds);
                                    st->doy = (seconds - year_sec) / 86400;
                                    st->isdst = 0;
                                    return;
                                } else {
                                    sec += adder;
                                }
                            }
                            return;
                        } else {
                            sec += adder;
                        }
                    }
                    return;
                } else {
                    sec += adder;
                }
            }
            return;
        } else {
            sec += adder;
        }
    }
}