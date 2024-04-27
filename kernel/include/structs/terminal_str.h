#pragma once

#include <structs/framebuffer_str.h>
#include <graphics/graphics.h>
#include <stdint.h>

typedef enum {
    TERM        = 0x0,
    GUI         = 0x1,
    INFO        = 0x2,
    UNSET       = 0x3,
} TERMINAL_MODE;

typedef struct {
    FRAMEBUFFER framebuffer;
    uint32_t background_color;
    uint32_t foreground_color;
    uint32_t width;
    uint32_t height;
    IVEC2 cursor_pos;
    TERMINAL_MODE mode;
} TERMINAL;