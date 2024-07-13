/**
 * @file stack_walk.h
 * @author Zack Bostock 
 * @brief Information pertaining to the stack walk utility 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <globals.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* ----------------------------- STATIC GLOBALS ----------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void stack_walk(void *ebp, uint8_t depth);

/* --------------------------- EXTERNALLY DEFINED --------------------------- */
void walk_memory(void *rsp, uint8_t depth);