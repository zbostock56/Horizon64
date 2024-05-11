#pragma once

#include <globals.h>
#include <stdarg.h>

static const char CONVERSION_TABLE[] = "0123456789abcdef";

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void init_terminal(FRAMEBUFFER fb);
void terminal_set_foreground(TERMINAL *t, FRAMEBUFFER_COLORS color);
void terminal_set_background(TERMINAL *t, FRAMEBUFFER_COLORS color);
void set_terminal_cursor_pos(TERMINAL *t, uint32_t x, uint32_t y);
void terminal_push_cursor(TERMINAL *t);
void terminal_putc(TERMINAL *t, uint8_t c);
void terminal_puts(TERMINAL *t, const char *s);
void terminal_printf(TERMINAL *t, const char *format, ...);

/* --------------------------- EXTERNALLY DEFINED --------------------------- */
void fb_putc(FRAMEBUFFER *fb, uint32_t x, uint32_t y, uint32_t fgcolor,
              uint32_t bgcolor, uint8_t ch);
