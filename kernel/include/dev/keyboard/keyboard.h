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

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* ----------------------------- STATIC GLOBALS ----------------------------- */
static volatile KEYBOARD_MOUSE keyboard = {0};

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */

/* --------------------------- EXTERNALLY DEFINED --------------------------- */
void irq_register_handler(int irq, IRQ_HANDLER handler);
char get_character_from_scancode(uint8_t scancode, uint8_t shift,
                                 uint8_t caps_lock);
void pic_unmask(int irq);
void terminal_putc(TERMINAL *t, uint8_t c);
