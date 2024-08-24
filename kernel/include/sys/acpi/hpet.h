/**
 * @file hpet.h
 * @author Zack Bostock
 * @brief Information pertaining to the High Precision Event Timer (HPET)
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <const.h>

#include <common/kprint.h>

#include <sys/mmu.h>
#include <sys/acpi/acpi.h>
#include <structs/hpet_str.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
/**
 * @brief Helper constant for determining operation mode of the HPET
 */
#define LONG_MODE_OPERATION (1)

/**
 * @brief Helper constant for determining the main counter's value
 */
#define NO_HPET_INITIALIZED (-1)

/**
 * @brief General Configuration Register values
 */
#define ENABLE_CNF          (0)
#define LEG_RT_CNF          (1)

/**
 * @brief General Capabilities and ID Register values
 */
#define COUNT_SIZE_CAP      (13)
#define LEG_RT_CAP          (15)

/**
 * @brief Constants for checking if the number of timers are valid
 */
#define HPET_MIN_COMPARATOR (3)
#define HPET_MAX_COMPARATOR (32)

/**
 * @brief Constants pertaining to the Timer Configuration and Capacity Register
 */
#define TMR_FSB_INT_DEL_CAP (15)
#define TMR_FSB_EN_CNF      (14)
#define TMR_32MODE_CNF      (8)
#define TMR_VAL_SET_CNF     (6)
#define TMR_SIZE_CAP        (5)
#define TMR_PER_INT_CAP     (4)
#define TMR_TYPE_CNF        (3)
#define TMR_INT_ENB_CNF     (2)
#define TMR_INT_TYPE_CNF    (1)

/* -------------------------------- GLOBALS --------------------------------- */
extern HPET *hpet;

/* --------------------------------- MACROS --------------------------------- */
/**
 * @brief Used to set bits in the General Configuration Register
 */
#define GEN_CONFIG_SET(bit, in) {                                           \
    if (in) {                                                               \
        (in)->general_config |= (1 << (bit));                               \
    }                                                                       \
}

/**
 * @brief Used to unset bits in the General Configuration Register
 */
#define GEN_CONFIG_UNSET(bit, in) {                                         \
    if (in) {                                                               \
        (in)->general_config &= (0 << (bit));                               \
    }                                                                       \
}

/**
 * @brief Used to read the Configuration and Capabilities register for an
 *        HPET timer
 */
#define HPET_TIMER_READ_CONFIG_CAP(bit, timer)                              \
    ((timer)->config_and_capabilities & (1 << (bit)))

/**
 * @brief Used to read the General Capabilities Register
 */
#define GEN_CAPABILITIES_READ(bit, in)                                      \
    ((in)->general_capabilities & (1 << (bit)))

#define HPET_NUM_COMPARATOR_CHECK(num)                                      \
    ((num) < HPET_MAX_COMPARATOR && (num) > HPET_MIN_COMPARATOR)

/* --------------------------- INTERNALLY DEFINED --------------------------- */
STATUS hpet_init();
