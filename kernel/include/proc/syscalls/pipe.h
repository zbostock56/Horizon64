/**
 * @file pipe.h
 * @author Zack Bostock
 * @brief Functionality pertaining to pipe system call
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
int64_t sys_pipe(int32_t pipefd[2]);

