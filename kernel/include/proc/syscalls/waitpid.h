/**
 * @file waitpid.h
 * @author Zack Bostock
 * @brief Functionality pertaining to waitpid system call
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
int64_t sys_waitpid(int64_t pid, int32_t *status, int32_t flags);

