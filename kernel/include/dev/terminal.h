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

#include <structs/terminal_str.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */
extern TERM_CURSOR_STATUS cursor_visible;

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void init_terminal(struct limine_framebuffer *fb);
void terminal_start();
void terminal_clear(TERM_MODE mode);
void terminal_print(TERM_MODE mode, uint8_t c);
void terminal_set_foreground(TERM_MODE mode, FRAMEBUFFER_COLORS color);
void terminal_set_background(TERM_MODE mode, FRAMEBUFFER_COLORS color);
void set_terminal_cursor_pos(TERMINAL *t, uint32_t x, uint32_t y);
void terminal_push_cursor(TERMINAL *t);
void terminal_pop_cursor(TERMINAL *t);
STATUS terminal_parse_cmd(TERMINAL *curr, uint8_t byte);
void terminal_putc(TERM_MODE mode, uint8_t c);
void terminal_puts(TERM_MODE mode, const char *s);
void terminal_scroll(TERMINAL *t);
void terminal_set_cursor(uint8_t c);
TERM_MODE terminal_get_mode();
void terminal_get_winsize(WINDOW_SIZE *ws);
STATUS terminal_set_winsize(WINDOW_SIZE *ws);
STATUS terminal_refresh(TERM_MODE mode);
uint8_t terminal_need_redraw();
void terminal_set_redraw(uint8_t a);
