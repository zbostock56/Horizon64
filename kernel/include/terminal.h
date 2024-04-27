#pragma once

#include <globals.h>

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void init_terminal(FRAMEBUFFER fb);
void terminal_set_foreground(TERMINAL *t, FRAMEBUFFER_COLORS color);
void terminal_set_background(TERMINAL *t, FRAMEBUFFER_COLORS color);
void set_terminal_cursor_pos(TERMINAL *t, uint32_t x, uint32_t y);
void terminal_push_cursor(TERMINAL *t);
void terminal_putc(TERMINAL *t, uint8_t c);
void terminal_puts(TERMINAL *t, const char *s);

/* --------------------------- EXTERNALLY DEFINED --------------------------- */
void fb_putc(FRAMEBUFFER *fb, uint32_t x, uint32_t y, uint32_t fgcolor,
              uint32_t bgcolor, uint8_t ch);