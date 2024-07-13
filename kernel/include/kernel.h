/**
 * @file kernel.h
 * @author Zack Bostock 
 * @brief Information pertaining to the starting of the kernel 
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

/* --------------------------- EXTERNALLY DEFINED --------------------------- */
void system_init();
void terminal_puts(TERMINAL *t, const char *s);