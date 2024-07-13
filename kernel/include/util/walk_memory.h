/**
 * @file walk_memory.h
 * @author Zack Bostock 
 * @brief Information pertaining to the memory walker utility 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <globals.h>

/* ----------------------------- STATIC GLOBALS ----------------------------- */

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void walk_memory(void *rsp, uint8_t depth);

/* --------------------------- EXTERNALLY DEFINED --------------------------- */