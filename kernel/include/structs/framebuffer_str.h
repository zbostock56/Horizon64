/**
 * @file framebuffer_str.h
 * @author Zack Bostock
 * @brief Structs utilized by the framebuffer functionality
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>
#include <graphics/graphics.h>

typedef struct {
    void *base;
    void *backbuffer;
    void *swapbuffer;
    void *background_buffer;
    uint64_t height;
    uint64_t width;
    uint64_t pitch;
    uint16_t bpp;           /* Bits per pixel */
    uint64_t backbuffer_length;
} FRAMEBUFFER;

/**
 * @brief Default 4 bit colors
 * @ref https://en.wikipedia.org/wiki/ANSI_escape_code
 */
typedef enum {
    COLOR_BLACK             = 0x000000,
    COLOR_RED               = 0xAA0000,
    COLOR_GREEN             = 0x00AA00,
    COLOR_YELLOW            = 0xAAAA00,
    COLOR_BROWN             = 0xAA5500,
    COLOR_BLUE              = 0x0000AA,
    COLOR_MAGENTA           = 0xAA00AA,
    COLOR_CYAN              = 0x00AAAA,
    COLOR_WHITE             = 0xAAAAAA,
    COLOR_BRIGHT_BLACK      = 0x555555,
    COLOR_BRIGHT_RED        = 0xFFAAAA,
    COLOR_BRIGHT_GREEN      = 0xAAFFAA,
    COLOR_BRIGHT_YELLOW     = 0xFFFFAA,
    COLOR_BRIGHT_BLUE       = 0xAAAAFF,
    COLOR_BRIGHT_MAGENTA    = 0xFFAAFF,
    COLOR_BRIGHT_CYAN       = 0xAAFFFF,
    COLOR_BRIGHT_WHITE      = 0xFFFFFF
} FRAMEBUFFER_COLORS;
