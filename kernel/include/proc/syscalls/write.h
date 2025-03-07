/**
 * @file write.h
 * @author Zack Bostock
 * @brief Functionality pertaining to write system call
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
int64_t sys_write(int64_t fh, void *buff, size_t count);

