/**
 * @file fstat.h
 * @author Zack Bostock
 * @brief Functionality pertaining to fstat system call
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
int64_t sys_fstat(int64_t handle, int64_t statbuf);

