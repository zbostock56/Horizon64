/**
 * @file scancodes.h
 * @author Zack Bostock
 * @brief Information pertaining to PS2 keyboard scan codes
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>
#include <const.h>

/* Reference: https://wiki.osdev.org/PS2_Keyboard */

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */


/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
char get_character_from_scancode(uint8_t scancode, uint8_t shift,
                                 uint8_t caps_lock);
