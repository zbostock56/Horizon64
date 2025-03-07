/**
 * @file faccessat.h
 * @author Zack Bostock
 * @brief Functionality pertaining to faccessat system call
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
int64_t sys_faccessat(int64_t dirfh, const char *path, uint64_t mode,
                      uint64_t flags);

