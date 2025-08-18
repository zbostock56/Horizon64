/**
 * @file framebuffer.h
 * @author Zack Bostock
 * @brief Information pertaining to framebuffer functionality
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <globals.h>

#include <structs/framebuffer_str.h>
#include <structs/terminal_str.h>


/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
STATUS fb_init(FRAMEBUFFER *fb, struct limine_framebuffer *f);
void fb_putpixel(FRAMEBUFFER *fb, uint32_t x, uint32_t y, uint32_t color);
uint32_t fb_getpixel(FRAMEBUFFER *fb, uint32_t x, uint32_t y);
void fb_putc(FRAMEBUFFER *fb, uint32_t x, uint32_t y, uint32_t fgcolor,
              uint32_t bgcolor, uint8_t ch, uint8_t is_bold);
void fb_refresh(FRAMEBUFFER *fb);
void fb_draw_characters(FRAMEBUFFER *fb, uint32_t fg, uint32_t bg,
                        const char *str);
