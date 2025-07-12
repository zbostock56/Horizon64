/**
 * @file mkdirat.h
 * @author Zack Bostock
 * @brief Functionality pertaining to mkdirat system call
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
int64_t sys_mkdirat(int64_t dirfh, const char *pathname, uint64_t mode);

