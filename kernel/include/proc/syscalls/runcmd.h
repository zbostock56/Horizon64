/**
 * @file runcmd.h
 * @author Zack Bostock
 * @brief Functionality pertaining to runcmd system call
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
int64_t sys_runcmd(char *cmd);

