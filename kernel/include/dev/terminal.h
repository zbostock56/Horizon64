/**
 * @file terminal.h
 * @author Zack Bostock
 * @brief Information pertaining to terminal functionality
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <globals.h>
#include <stdarg.h>

#include <graphics/framebuffer.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
#define TERMINAL_LEFT_OFFSET (1)

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void init_terminal(FRAMEBUFFER fb);
void terminal_set_foreground(TERMINAL *t, FRAMEBUFFER_COLORS color);
void terminal_set_background(TERMINAL *t, FRAMEBUFFER_COLORS color);
void set_terminal_cursor_pos(TERMINAL *t, uint32_t x, uint32_t y);
void terminal_push_cursor(TERMINAL *t);
void terminal_scroll(TERMINAL *t);
void terminal_clear(TERMINAL *t);
void terminal_putc(TERMINAL *t, uint8_t c);
void terminal_puts(TERMINAL *t, const char *s);
void terminal_printf(TERMINAL *t, const char *format, ...);
