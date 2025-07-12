/**
 * @file fstatat.h
 * @author Zack Bostock
 * @brief Functionality pertaining to fstatat system call
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
int64_t sys_fstatat(int64_t dirfh, const char *path, int64_t statbuf,
                    int64_t flags);

