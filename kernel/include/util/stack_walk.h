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

#include <util/walk_memory.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void stack_walk(void *ebp, uint8_t depth);
