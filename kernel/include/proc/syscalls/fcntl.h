/**
 * @file fnctl.h
 * @author Zack Bostock
 * @brief Functionality pertaining to fnctl system call
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
int64_t sys_fcntl(int64_t fd, int64_t request, int64_t args);

