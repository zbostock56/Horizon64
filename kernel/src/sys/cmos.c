/**
 * @file cmos.c
 * @author Zack Bostock
 * @brief Functionality pertaining to CMOS and RTC (Real-Time Clock) 
 * @verbatim 
 * "CMOS" is a tiny bit of very low power static memory that lives on the
 * same chip as the Real-Time Clock (RTC). It was introduced to IBM PC AT in
 * 1984 which used Motorola MC146818A RTC.
 * @ref https://wiki.osdev.org/CMOS 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <sys/cmos.h>

#define CURRENT_YEAR        (2024)

/* Ideally, set by the ACPI table parsing code if possible */
int century_register = 0x0;

static CMOS boot_time;

/**
 * @brief Get the value of a register
 * 
 * @param reg Register to get value from
 * @return uint8_t Value from the register
 */
static inline uint8_t get_reg_value(uint8_t reg) {
    outb(CMOS_CMD_PORT, reg | (1 << 7));
    io_wait();
    return inb(CMOS_DATA_PORT);
}

/**
 * @brief Helper function to wait for the CMOS/RTC to process
 * @verbatim 
 * Here, we check to see if the "Update in Progress" flag is still set on the
 * chip and wait until it isn't anymore before proceeding.
 * @return int 1 if busy, 0 if not busy
 */
inline static uint8_t wait() {
    outb(CMOS_CMD_PORT, CMOS_REG_STA);
    return (inb(CMOS_DATA_PORT) & 0x80);
}

/**
 * @brief Helper function to find the time from the registers in a reliable
 *        fashion
 * 
 * @return CMOS Structure with the current time housed
 */
static CMOS read_time() {
    CMOS cur = {0};
    CMOS prev = {0};

    while (wait())
        ;
    GET_TIME(cur);

    /* Continually get the values until they match to get around the headache */
    /* of CMOS returning weird values.                                        */
    do {
        memcpy(&prev, &cur, sizeof(CMOS));
        while (wait())
            ;
        GET_TIME(cur);
    } while (!CHECK_TIME_EQ(cur, prev));

    uint8_t b = get_reg_value(CMOS_REG_STB);

    /* Check to see if the values need to be converted */
    if (!(b & 0x04)) {
        cur.seconds = TO_BINARY(cur.seconds);
        cur.minutes = TO_BINARY(cur.minutes);
        cur.hours = TO_BINARY(cur.hours);
        cur.weekdays = TO_BINARY(cur.weekdays);
        cur.day = TO_BINARY(cur.day);
        cur.month = TO_BINARY(cur.month);
        cur.year = TO_BINARY(cur.year);
        cur.century = TO_BINARY(cur.century);
    }

    /* Convert 12 hour to 24 hour if neccessary */
    if (!(b & 0x02) && (cur.hours & 0x80)) {
        cur.hours = ((cur.hours & 0x7F) + 12) % 24;
    }

    /* Calculate the full 4-digit year */
    if (cur.year < 100) {
        if (century_register != 0) {
            cur.year += cur.century * 100;
        } else {
            cur.year += (CURRENT_YEAR / 100) * 100;
            if (cur.year < CURRENT_YEAR) {
                cur.year += 100;
            }
        }
    }

    return cur;
}

/**
 * @brief Main CMOS/RTC initialization function
 */
void cmos_init() {
    klogi("INIT CMOS: starting...\n");
    boot_time = read_time();
    klogi("Boot time: %d/%d/%d at %d:%d:%d\n",
           boot_time.month, boot_time.day, boot_time.year, boot_time.hours,
           boot_time.minutes, boot_time.seconds);
    klogi("INIT CMOS: finished...\n");
}