/**
 * @file pf_decode.h
 * @author Zack Bostock
 * @brief Information pertaining to decoding #PF error codes
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
void pf_decode(uint64_t error_code, uint64_t cr2);