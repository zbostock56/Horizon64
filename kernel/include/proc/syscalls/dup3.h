/**
 * @file dup3.h
 * @author Zack Bostock
 * @brief Functionality pertaining to dup3 system call
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
int64_t sys_dup3(int64_t fh, int64_t newfh, int64_t flags);

