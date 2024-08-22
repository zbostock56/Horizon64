/**
 * @file errno.h
 * @author Zack Bostock
 * @brief Information pertianing to system error codes
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>

typedef uint8_t ERRNO;

#define ERR_NO_ERR                              (1)
#define ERR_SERIAL_FAULTY                       (2)
#define ERR_PSF1_ERR                            (3)
