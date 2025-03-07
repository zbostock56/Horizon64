/**
 * @file terminal_str.h
 * @author Zack Bostock
 * @brief Structs pertaining to the terminal
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <structs/framebuffer_str.h>
#include <graphics/graphics.h>
#include <stdint.h>

#define TERM_IS_BOLD        (1)
#define TERM_NOT_BOLD       (0)
typedef uint8_t TERM_BOLD;

typedef enum {
    TERM_MODE_TERM = 0x0,
    TERM_MODE_GUI,
    TERM_MODE_INFO,
    TERM_MODE_UNSET,
} TERM_MODE;

typedef enum {
    TERM_CURSOR_INVISIBLE = 0x0,
    TERM_CURSOR_VISIBLE,
    TERM_CURSOR_HIDE
} TERM_CURSOR_STATUS;

typedef enum {
    TERM_STATE_UNKNOWN = 0x0,
    TERM_STATE_IDLE,
    TERM_STATE_CMD,
    TERM_STATE_PARAM,
    TERM_STATE_HYPERLINK_HEADER,
    TERM_STATE_HYPERLINK_URL,
    TERM_STATE_HYPERLINK_TEXT,
    TERM_STATE_HYPERLINK_TAIL,
} TERM_STATE;

typedef struct {
    FRAMEBUFFER framebuffer;
    uint32_t background_color;
    uint32_t foreground_color;
    uint32_t width;             /* In units of characters, not pixels */
    uint32_t height;            /* In units of characters, not pixels */
    IVEC2 cursor_pos;           /* In units of characters, not pixels */
    TERM_MODE mode;
    TERM_BOLD is_bold;
    TERM_STATE state;

    int cparams[16];
    int cparamcount;

    uint8_t last_char;
    uint8_t last_qu_char;
    uint8_t skip_left;
} TERMINAL;

typedef struct {
    uint16_t row;           /* Rows, in characters */
    uint16_t col;           /* Columns, in characters */
    uint16_t xpixel;        /* Horizontal size, in pixels */
    uint16_t ypixel;        /* Vertical size, in pixels */
} __attribute__((packed)) WINDOW_SIZE;
