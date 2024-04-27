#pragma once

#include <stdint.h>
#include <graphics/graphics.h>

typedef struct {
    void *base;
    void *backbuffer;
    void *swapbuffer;
    uint64_t height;
    uint64_t width;
    uint64_t pitch;
    uint16_t bpp;
} FRAMEBUFFER;

typedef enum {
    COLOR_BLACK         = 0x000000,
    COLOR_RED           = 0xAA0000,
    COLOR_GREEN         = 0x00AA00,
    COLOR_YELLOW        = 0xAAAA00,
    COLOR_BROWN         = 0xAA5500,
    COLOR_BLUE          = 0x0000AA,
    COLOR_MAGENTA       = 0xAA00AA,
    COLOR_CYAN          = 0x00AAAA,
    COLOR_WHITE         = 0xAAAAAA,
} FRAMEBUFFER_COLORS;