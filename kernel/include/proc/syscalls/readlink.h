/**
 * @file readlink.h
 * @author Zack Bostock
 * @brief Functionality pertaining to readlink system call
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
int64_t sys_readlink(int64_t dirfh, const char *path, void *buffer,
                     size_t max_size);

