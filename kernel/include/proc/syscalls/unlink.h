/**
 * @file unlink.h
 * @author Zack Bostock
 * @brief Functionality pertaining to unlink system call
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
int64_t sys_unlink(char *path);

