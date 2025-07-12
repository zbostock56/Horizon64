/**
 * @file ioctl.h
 * @author Zack Bostock
 * @brief Functionality pertaining to ioctl system call
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
int64_t sys_ioctl(int64_t fh, int64_t request, int64_t arg);

