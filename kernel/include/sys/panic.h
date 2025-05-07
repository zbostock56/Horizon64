/**
 * @file panic.h
 * @author Zack Bostock
 * @brief Information about kernel panics
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
void backtrace(uint64_t rip);