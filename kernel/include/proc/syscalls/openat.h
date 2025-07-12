/**
 * @file openat.h
 * @author Zack Bostock
 * @brief Functionality pertaining to openat system call
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
int64_t sys_openat(int64_t dirfh, char *path, int64_t flags, int64_t mode);

