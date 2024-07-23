/**
 * @file keyboard.h
 * @author Zack Bostock
 * @brief Information pertaining to keyboard functionality
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <globals.h>

#include <structs/ps2_str.h>
#include <structs/keyboard_str.h>
#include <structs/regs_str.h>

#include <sys/asm.h>
#include <sys/interrupts/irq.h>
#include <sys/tick/pic.h>

#include <dev/keyboard/scancodes.h>
#include <dev/terminal.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void keyboard_set_key(uint8_t state, uint8_t scancode);
void keyboard_init();
