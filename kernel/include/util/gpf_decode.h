/**
 * @file gpf_decode.h
 * @author Zack Bostock
 * @brief Information pertaining to decoding General Protection Fault error
 *        codes
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <stdint.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void gpf_decode(uint64_t error_code);