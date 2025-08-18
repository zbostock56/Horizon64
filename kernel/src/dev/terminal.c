/**
 * @file terminal.c
 * @author Zack Bostock
 * @brief Helpers and initialization of the terminal
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <kconfig.h>

#include <common/lock.h>
#include <common/kprint.h>
#include <common/math.h>
#include <common/memory.h>
#include <common/string.h>

#include <graphics/framebuffer.h>

#include <dev/terminal.h>
#include <dev/serial.h>

/**
 * @brief Global terminal state
 */
static LOCK term_lock = LOCK_NEW;
static TERM_MODE term_mode = TERM_MODE_UNSET;
static volatile uint8_t term_need_redrawn = FALSE;
static TERMINAL term_info = {0};
static TERMINAL term_cli = {0};
static uint8_t term_cursor = 0;
static volatile uint8_t term_char_print = 1;
static TERMINAL_STATS global_stats = {0};

/* Global terminal termios settings */
static TERMIOS global_termios = {
    .c_iflag = BRKINT | ICRNL,
    .c_oflag = OPOST | ONLCR,
    .c_cflag = CS8 | CREAD,
    .c_lflag = ISIG | ICANON | ECHO | ECHOE | ECHOK,
    .c_cc = {
        [VINTR]  = 0x03,    /* Ctrl-C */
        [VQUIT]  = 0x1C,    /* Ctrl-\ */
        [VERASE] = 0x08,    /* Backspace */
        [VKILL]  = 0x15,    /* Ctrl-U */
        [VEOF]   = 0x04,    /* Ctrl-D */
        [VEOL]   = 0x00,    /* No end of line char */
        [VMIN]   = 1,       /* Minimum chars for non-canonical read */
        [VTIME]  = 0,       /* Timeout for non-canonical read */
        [VSTART] = 0x11,    /* Ctrl-Q */
        [VSTOP]  = 0x13,    /* Ctrl-S */
    }
};

static const uint32_t four_bit_colors[16] = {
    COLOR_BLACK,          /* 0: Black */
    COLOR_RED,            /* 1: Red */
    COLOR_GREEN,          /* 2: Green */
    COLOR_YELLOW,         /* 3: Yellow */
    COLOR_BLUE,           /* 4: Blue */
    COLOR_MAGENTA,        /* 5: Magenta */
    COLOR_CYAN,           /* 6: Cyan */
    COLOR_WHITE,          /* 7: White */
    COLOR_BRIGHT_BLACK,   /* 8: Bright Black (Gray) */
    COLOR_BRIGHT_RED,     /* 9: Bright Red */
    COLOR_BRIGHT_GREEN,   /* 10: Bright Green */
    COLOR_BRIGHT_YELLOW,  /* 11: Bright Yellow */
    COLOR_BRIGHT_BLUE,    /* 12: Bright Blue */
    COLOR_BRIGHT_MAGENTA, /* 13: Bright Magenta */
    COLOR_BRIGHT_CYAN,    /* 14: Bright Cyan */
    COLOR_BRIGHT_WHITE    /* 15: Bright White */
};

#if FRAMEBUFFER_LOGGING
static const uint8_t DEFAULT_CHAR_PRINT = 1;
#else
static const uint8_t DEFAULT_CHAR_PRINT = 0;
#endif

TERM_CURSOR_STATUS cursor_visible = TERM_CURSOR_INVISIBLE;

/* -------------------------- Forward declarations -------------------------- */
static STATUS terminal_validate_params(const TERMINAL *term);
static void terminal_stats_update(TERMINAL *curr, TERM_OPERATION op);
static STATUS terminal_handle_control_char(TERMINAL *curr, uint8_t c);
static void terminal_handle_tab(TERMINAL *curr, uint32_t tab_width);
static void terminal_handle_backspace(TERMINAL *curr);
static STATUS terminal_resize_check(TERMINAL *curr);
static void terminal_update_cursor_visibility(TERMINAL *curr, TERM_MODE mode);
static STATUS terminal_parse_csi_sequence(TERMINAL *curr, uint8_t byte);
static STATUS terminal_parse_osc_sequence(TERMINAL *curr, uint8_t byte);
static STATUS terminal_execute_osc_command(TERMINAL *curr);

/* Terminal command implementations */
static STATUS terminal_cursor_up(TERMINAL *curr, int count);
static STATUS terminal_cursor_down(TERMINAL *curr, int count);
static STATUS terminal_cursor_forward(TERMINAL *curr, int count);
static STATUS terminal_cursor_backward(TERMINAL *curr, int count);
static STATUS terminal_cursor_position(TERMINAL *curr, int row, int col);
static STATUS terminal_save_cursor(TERMINAL *curr);
static STATUS terminal_restore_cursor(TERMINAL *curr);
static STATUS terminal_set_graphics(TERMINAL *curr, int *params, int param_count);
#if TERM_OSC
static STATUS terminal_set_window_title(TERMINAL *curr, const char *title);
static STATUS terminal_set_icon_name(TERMINAL *curr, const char *name);
static STATUS terminal_set_color_palette(TERMINAL *curr, const char *spec);
static STATUS terminal_set_dynamic_color(TERMINAL *curr, int param, const char *color);
#endif
static STATUS terminal_erase_display(TERMINAL *curr, int mode);
static STATUS terminal_erase_line(TERMINAL *curr, int mode);


/**
 * @brief Checks if the given coordinates are within terminal bounds.
 *
 * @param term Pointer to the TERMINAL structure.
 * @param x X-coordinate to check.
 * @param y Y-coordinate to check.
 * @return 1 if coordinates are valid, 0 otherwise.
 */
static inline uint8_t terminal_coords_valid(const TERMINAL *term, int x, int y) {
    return term && x >= 0 && y >= 0 &&
           x < (int)term->width && y < (int)term->height;
}

/**
 * @brief Clamps the given coordinates to stay within terminal bounds.
 *
 * @param term Pointer to the TERMINAL structure.
 * @param x Pointer to the x-coordinate, modified in-place.
 * @param y Pointer to the y-coordinate, modified in-place.
 */
static inline void terminal_clamp_coords(const TERMINAL *term, int *x, int *y) {
    if (!term || !x || !y) return;
    *x = (*x < 0) ? 0 : ((*x >= (int)term->width) ? (int)term->width - 1 : *x);
    *y = (*y < 0) ? 0 : ((*y >= (int)term->height) ? (int)term->height - 1 : *y);
}

/**
 * @brief Converts terminal coordinates to a linear framebuffer index.
 *
 * @param term Pointer to the TERMINAL structure.
 * @param x X-coordinate.
 * @param y Y-coordinate.
 * @return Linear index into the framebuffer.
 */
static inline size_t terminal_coord_to_index(const TERMINAL *term, int x,
                                             int y) {
    if (!terminal_coords_valid(term, x, y)) return 0;
    return (size_t)(y * term->width + x);
}

/**
 * @brief Converts a linear framebuffer index to terminal coordinates.
 *
 * @param term Pointer to the TERMINAL structure.
 * @param index Linear index.
 * @param x Pointer to store the resulting x-coordinate.
 * @param y Pointer to store the resulting y-coordinate.
 */
static inline void terminal_index_to_coord(const TERMINAL *term, size_t index,
                                           int *x, int *y) {
    if (!term || !x || !y) return;
    *x = (int)(index % term->width);
    *y = (int)(index / term->width);
}

/**
 * @brief Validates the internal state and dimensions of a terminal.
 *
 * @param term Pointer to the TERMINAL structure.
 * @return SYS_OK if valid, SYS_ERR otherwise.
 */
static STATUS terminal_validate_params(const TERMINAL *term) {
    if (!term) {
        return SYS_ERR;
    }

    if (term->state == TERM_STATE_UNKNOWN) {
        return SYS_ERR;
    }

    if (!term->framebuffer.base) {
        return SYS_ERR;
    }

    if (term->width == 0 || term->height == 0) {
        return SYS_ERR;
    }

    return SYS_OK;
}

/**
 * @brief Updates terminal and global statistics for a specific operation.
 *
 * @param curr Pointer to the TERMINAL structure.
 * @param op Operation to record in stats.
 */
static void terminal_stats_update(TERMINAL *curr, TERM_OPERATION op) {
    if (!curr) return;

    switch (op) {
        case TERM_OP_CHAR_WRITE:
            curr->stats.chars_written++;
            global_stats.chars_written++;
            break;
        case TERM_OP_LINE_FEED:
            curr->stats.lines_written++;
            global_stats.lines_written++;
            break;
        case TERM_OP_SCROLL:
            curr->stats.scroll_operations++;
            global_stats.scroll_operations++;
            break;
        case TERM_OP_ESCAPE_SEQ:
            curr->stats.escape_sequences++;
            global_stats.escape_sequences++;
            break;
        case TERM_OP_ERROR:
            curr->stats.errors++;
            global_stats.errors++;
            break;
        case TERM_OP_BELL:
            curr->stats.bell_operations++;
            global_stats.bell_operations++;
            break;
        default:
            break;
    }
}

/**
 * @brief Handles a control character in the terminal input stream.
 *
 * @param curr Pointer to the TERMINAL structure.
 * @param c Control character (0x00–0x1F).
 * @return SYS_OK if handled successfully, SYS_ERR on error.
 */
static STATUS terminal_handle_control_char(TERMINAL *curr, uint8_t c) {
    if (!curr) return SYS_ERR;

    switch (c) {
        case '\0':  /* NULL */
            return SYS_OK;

        case '\a':  /* BELL */
            /* TODO: Implement bell/beep functionality */
            terminal_stats_update(curr, TERM_OP_BELL);
            return SYS_OK;

        case '\b':  /* BACKSPACE */
            terminal_handle_backspace(curr);
            return SYS_OK;

        case '\t':  /* HORIZONTAL TAB */
            terminal_handle_tab(curr, TERMINAL_DEFAULT_TAB_WIDTH);
            return SYS_OK;

        case '\n':  /* LINE FEED */
        case '\v':  /* VERTICAL TAB */
        case '\f':  /* FORM FEED */
            curr->cursor_pos.x = 0;
            curr->cursor_pos.y++;
            terminal_stats_update(curr, TERM_OP_LINE_FEED);
            return SYS_OK;

        case '\r':  /* CARRIAGE RETURN */
            curr->cursor_pos.x = 0;
            return SYS_OK;

        case ESC_START:  /* ESCAPE */
            curr->state = TERM_STATE_CMD;
            curr->last_qu_char = TRUE;
            return SYS_OK;

        default:
            /* Other control characters are generally ignored */
            return SYS_OK;
    }
}

/**
 * @brief Handles tab character by moving the cursor forward to next tab stop.
 *
 * @param curr Pointer to the TERMINAL structure.
 * @param tab_width Width between tab stops.
 */
static void terminal_handle_tab(TERMINAL *curr, uint32_t tab_width) {
    if (!curr) return;

    uint32_t spaces_to_add = tab_width - (curr->cursor_pos.x % tab_width);
    curr->cursor_pos.x += spaces_to_add;

    /* Handle line wrap */
    if (curr->cursor_pos.x >= (int)curr->width) {
        curr->cursor_pos.x = 0;
        curr->cursor_pos.y++;
    }
}

/**
 * @brief Handles backspace character by moving the cursor back and clearing.
 *
 * @param curr Pointer to the TERMINAL structure.
 */
static void terminal_handle_backspace(TERMINAL *curr) {
    if (!curr) return;

    if (curr->cursor_pos.x > 0) {
        curr->cursor_pos.x--;
        /* Clear the character at the cursor position */
        fb_putc(&(curr->framebuffer),
                curr->cursor_pos.x * PSF1_FONT_WIDTH,
                curr->cursor_pos.y * font.header->character_size,
                curr->foreground_color, curr->background_color, ' ', FALSE);
    } else if (curr->cursor_pos.y > 0) {
        /* Move to end of previous line */
        curr->cursor_pos.y--;
        curr->cursor_pos.x = curr->width - 1;
    }
}

/**
 * @brief Checks if the terminal needs to scroll or wrap and handles it.
 *
 * @param curr Pointer to the TERMINAL structure.
 * @return SYS_OK if successful, SYS_ERR on error.
 */
static STATUS terminal_resize_check(TERMINAL *curr) {
    if (!curr) return SYS_ERR;

    /* Check if cursor is out of bounds and handle scrolling */
    while (curr->cursor_pos.y >= (int)curr->height) {
        terminal_scroll(curr);
        curr->cursor_pos.y--;
        terminal_stats_update(curr, TERM_OP_SCROLL);
    }

    /* Handle horizontal wrapping */
    while (curr->cursor_pos.x >= (int)curr->width) {
        curr->cursor_pos.x -= curr->width;
        curr->cursor_pos.y++;
        if (curr->cursor_pos.y >= (int)curr->height) {
            terminal_scroll(curr);
            curr->cursor_pos.y--;
            terminal_stats_update(curr, TERM_OP_SCROLL);
        }
    }

    return SYS_OK;
}

/**
 * @brief Updates the cursor appearance based on visibility settings.
 *
 * @param curr Pointer to the TERMINAL structure.
 * @param mode Current terminal mode.
 */
static void terminal_update_cursor_visibility(TERMINAL *curr, TERM_MODE mode) {
    if (!curr || mode != term_mode) return;

    switch (cursor_visible) {
        case TERM_CURSOR_VISIBLE:
            term_cursor = '_';
            break;
        case TERM_CURSOR_BLOCK:
            term_cursor = 0xDB; /* Block character */
            break;
        case TERM_CURSOR_INVISIBLE:
            term_cursor = ' ';
            break;
        default:
            term_cursor = 0;
            break;
    }
}

/**
 * @brief Moves the terminal cursor up by a given count.
 *
 * @param curr Pointer to the TERMINAL structure.
 * @param count Number of rows to move up.
 * @return SYS_OK if successful, SYS_ERR on error.
 */
static STATUS terminal_cursor_up(TERMINAL *curr, int count) {
    if (!curr) return SYS_ERR;

    curr->cursor_pos.y = MAX(curr->cursor_pos.y - count, curr->scroll_top);
    return SYS_OK;
}

/**
 * @brief Moves the terminal cursor down by a given count.
 *
 * @param curr Pointer to the TERMINAL structure.
 * @param count Number of rows to move down.
 * @return SYS_OK if successful, SYS_ERR on error.
 */
static STATUS terminal_cursor_down(TERMINAL *curr, int count) {
    if (!curr) return SYS_ERR;

    curr->cursor_pos.y = MIN(curr->cursor_pos.y + count, curr->scroll_bottom);
    return SYS_OK;
}

/**
 * @brief Moves the terminal cursor forward (right) by a given count.
 *
 * @param curr Pointer to the TERMINAL structure.
 * @param count Number of columns to move right.
 * @return SYS_OK if successful, SYS_ERR on error.
 */
static STATUS terminal_cursor_forward(TERMINAL *curr, int count) {
    if (!curr) return SYS_ERR;

    curr->cursor_pos.x = MIN(curr->cursor_pos.x + count, (int)curr->width - 1);
    return SYS_OK;
}

/**
 * @brief Moves the terminal cursor backward (left) by a given count.
 *
 * @param curr Pointer to the TERMINAL structure.
 * @param count Number of columns to move left.
 * @return SYS_OK if successful, SYS_ERR on error.
 */
static STATUS terminal_cursor_backward(TERMINAL *curr, int count) {
    if (!curr) return SYS_ERR;

    curr->cursor_pos.x = MAX(curr->cursor_pos.x - count, 0);
    return SYS_OK;
}

/**
 * @brief Sets the terminal cursor to a specific row and column.
 *
 * @param curr Pointer to the TERMINAL structure.
 * @param row 1-based row position.
 * @param col 1-based column position.
 * @return SYS_OK if successful, SYS_ERR on error.
 */
static STATUS terminal_cursor_position(TERMINAL *curr, int row, int col) {
    if (!curr) return SYS_ERR;

    /* ANSI coordinates are 1-based, convert to 0-based */
    curr->cursor_pos.y = CLAMP(row - 1, 0, (int)curr->height - 1);
    curr->cursor_pos.x = CLAMP(col - 1, 0, (int)curr->width - 1);
    return SYS_OK;
}

/**
 * @brief Saves the current cursor position.
 *
 * @param curr Pointer to the TERMINAL structure.
 * @return SYS_OK if successful, SYS_ERR on error.
 */
static STATUS terminal_save_cursor(TERMINAL *curr) {
    if (!curr) return SYS_ERR;

    curr->saved_cursor_pos = curr->cursor_pos;
    return SYS_OK;
}

/**
 * @brief Restores the saved cursor position.
 *
 * @param curr Pointer to the TERMINAL structure.
 * @return SYS_OK if successful, SYS_ERR on error.
 */
static STATUS terminal_restore_cursor(TERMINAL *curr) {
    if (!curr) return SYS_ERR;

    curr->cursor_pos = curr->saved_cursor_pos;
    return SYS_OK;
}

/**
 * @brief Sets terminal text attributes like color, bold, underline, etc.
 *
 * @param curr Pointer to the TERMINAL structure.
 * @param params List of SGR parameters.
 * @param param_count Number of parameters in the list.
 * @return SYS_OK if successful, SYS_ERR on error.
 */
static STATUS terminal_set_graphics(TERMINAL *curr, int *params, int param_count) {
    if (!curr) return SYS_ERR;

    /* Handle no parameters (reset) */
    if (param_count == 0) {
        params = (int[]){0};
        param_count = 1;
    }

    for (int i = 0; i < param_count; i++) {
        int param = params[i];

        switch (param) {
            case 0:  /* Reset all attributes */
                curr->foreground_color = DEFAULT_FG;
                curr->background_color = DEFAULT_BG;
                curr->is_bold = FALSE;
                curr->is_underline = FALSE;
                curr->is_reverse = FALSE;
                break;

            case 1:  /* Bold */
                curr->is_bold = TRUE;
                break;

            case 4:  /* Underline */
                curr->is_underline = TRUE;
                break;

            case 7:  /* Reverse video */
                curr->is_reverse = TRUE;
                break;

            case 22: /* Normal intensity (not bold) */
                curr->is_bold = FALSE;
                break;

            case 24: /* Not underlined */
                curr->is_underline = FALSE;
                break;

            case 27: /* Not reversed */
                curr->is_reverse = FALSE;
                break;

            /* Foreground colors (30-37, 90-97) */
            case 30: case 31: case 32: case 33:
            case 34: case 35: case 36: case 37:
                curr->foreground_color = four_bit_colors[param - 30];
                break;

            case 90: case 91: case 92: case 93:
            case 94: case 95: case 96: case 97:
                curr->foreground_color = four_bit_colors[(param - 90) + 8];
                break;

            /* Background colors (40-47, 100-107) */
            case 40: case 41: case 42: case 43:
            case 44: case 45: case 46: case 47:
                curr->background_color = four_bit_colors[param - 40];
                break;

            case 100: case 101: case 102: case 103:
            case 104: case 105: case 106: case 107:
                curr->background_color = four_bit_colors[(param - 100) + 8];
                break;

            /* 256-color and RGB color support */
            case 38:  /* Set foreground color (extended) */
            case 48:  /* Set background color (extended) */
                /* TODO: Implement 256-color and RGB support */
                break;

            default:
                /* Unknown parameter, ignore */
                break;
        }
    }

    return SYS_OK;
}


#if TERM_OSC
/**
 * @brief Sets the window title via OSC (Operating System Command).
 *
 * @param curr Pointer to the TERMINAL structure.
 * @param title New window title.
 * @return SYS_OK if successful, SYS_ERR on error.
 */
static STATUS terminal_set_window_title(TERMINAL *curr, const char *title) {
    if (!curr || !title) return SYS_ERR;
    /* TODO: Implement window title setting */
    klogw("TERM: Setting window title is not yet supported\n");
    return SYS_OK;
}

/**
 * @brief Sets the icon name via OSC command.
 *
 * @param curr Pointer to the TERMINAL structure.
 * @param name New icon name.
 * @return SYS_OK if successful, SYS_ERR on error.
 */
static STATUS terminal_set_icon_name(TERMINAL *curr, const char *name) {
    if (!curr || !name) return SYS_ERR;
    /* TODO: Implement icon name setting */
    klogw("TERM: Setting icon name is not yet supported\n");
    return SYS_OK;
}

/**
 * @brief Sets the icon name via OSC command.
 *
 * @param curr Pointer to the TERMINAL structure.
 * @param name New icon name.
 * @return SYS_OK if successful, SYS_ERR on error.
 */
static STATUS terminal_set_color_palette(TERMINAL *curr, const char *spec) {
    if (!curr || !spec) return SYS_ERR;
    /* TODO: Implement color palette setting */
    klogw("TERM: Setting color palette is not yet supported\n");
    return SYS_OK;
}

/**
 * @brief Sets a dynamic color via OSC command.
 *
 * @param curr Pointer to the TERMINAL structure.
 * @param param Color index/parameter to set.
 * @param color String representing the color.
 * @return SYS_OK if successful, SYS_ERR on error.
 */
static STATUS terminal_set_dynamic_color(TERMINAL *curr, int param, const char *color) {
    (void) param;
    if (!curr || !color) return SYS_ERR;
    /* TODO: Implement dynamic color setting */
    klogw("TERM: Setting dynamic color is not yet supported\n");
    return SYS_OK;
}
#endif

/**
 * @brief Parses and executes CSI (Control Sequence Introducer) byte.
 *
 * @param curr Pointer to the TERMINAL structure.
 * @param byte Byte from input stream to parse.
 * @return SYS_OK if handled successfully, SYS_ERR on error.
 */
static STATUS terminal_parse_csi_sequence(TERMINAL *curr, uint8_t byte) {
    if (!curr) return SYS_ERR;

    /*
     * CSI sequences: ESC [ [parameters] [intermediate bytes] final byte
     * Parameters: 0-9, ; (semicolon)
     * Intermediate bytes: 0x20-0x2F (space to /)
     * Final byte: 0x40-0x7E (@ to ~)
     */

    if (byte >= '0' && byte <= '9') {
        /* Parameter digit */
        if (curr->cparamcount == 0) {
            curr->cparamcount = 1;
            curr->cparams[0] = 0;
        }

        int param_idx = curr->cparamcount - 1;
        if (param_idx < TERMINAL_MAX_PARAMS) {
            curr->cparams[param_idx] = curr->cparams[param_idx] * 10 + (byte - '0');
        }
        return SYS_OK;
    }

    if (byte == ';') {
        /* Parameter separator */
        if (curr->cparamcount < TERMINAL_MAX_PARAMS) {
            curr->cparamcount++;
            curr->cparams[curr->cparamcount - 1] = 0;
        }
        return SYS_OK;
    }

    if (byte >= 0x20 && byte <= 0x2F) {
        /* Intermeidate byte - ignore for now */
        return SYS_OK;
    }

    if (byte >= 0x40 && byte <= 0x7E) {
        /* Final byte - now execute command */
        STATUS result = SYS_OK;

        /* Set default parameter if none is provided */
        if (curr->cparamcount == 0) {
            curr->cparamcount = 1;
            /* This is typically the default for most commands */
            curr->cparams[0] = 1;
        }

        switch (byte) {
            case 'A': /* Cursor up */
                result = terminal_cursor_up(curr, curr->cparams[0]);
                break;
            case 'B': /* Cursor Down */
                result = terminal_cursor_down(curr, curr->cparams[0]);
                break;
            case 'C': /* Cursor Forward */
                result = terminal_cursor_forward(curr, curr->cparams[0]);
                break;
            case 'D': /* Cursor Backward */
                result = terminal_cursor_backward(curr, curr->cparams[0]);
                break;
            case 'H': /* Cursor Position */
            case 'f': /* Horizontal and Vertical Position */
                {
                    int row = (curr->cparamcount > 0) ? curr->cparams[0] : 1;
                    int col = (curr->cparamcount > 1) ? curr->cparams[1] : 1;
                    result = terminal_cursor_position(curr, row, col);
                }
                break;
            case 'J': /* Erase in Display */
                if (curr->cparams[0] == 2) {
                    terminal_clear(curr->mode);
                } else {
                    result = terminal_erase_display(curr, curr->cparams[0]);
                }
                break; 
            case 'K': /* Erase in Line */
                result = terminal_erase_line(curr, curr->cparams[0]);
                break;
            case 'm': /* Select Graphic Rendition (colors/attributes) */
                result = terminal_set_graphics(curr, curr->cparams,
                                               curr->cparamcount);
                break;
            case 's': /* Save Cursor Position */
                result = terminal_save_cursor(curr);
                break;
            case 'u': /* Restore Cursor Position */
                result = terminal_restore_cursor(curr);
                break;
            default:
                /* Unknown CSI sequence - just ignore it */
                klogw("TERM parse_csi_sequence: hit unrecognized CSI seq: '%c'\n",
                        byte);
                break;
        }

        /* Reset CSI state */
        curr->state = TERM_STATE_IDLE;
        curr->cparamcount = 0;
        terminal_stats_update(curr, TERM_OP_ESCAPE_SEQ);

        return result;
    }

    /* Invalid byte in CSI sequence */
    kloge("TERM parse_csi_sequence: Invalid byte '%c'\n", byte);
    curr->state = TERM_STATE_IDLE;
    curr->cparamcount = 0;
    terminal_stats_update(curr, TERM_OP_ERROR);
    return SYS_ERR;
}

/**
 * @brief Parses and executes OSC (Operating System Command) byte.
 *
 * @param curr Pointer to the TERMINAL structure.
 * @param byte Byte from input stream to parse.
 * @return SYS_OK if handled successfully, SYS_ERR on error.
 */
static STATUS terminal_parse_osc_sequence(TERMINAL *curr, uint8_t byte) {
    if (!curr) return SYS_ERR;

    switch (curr->state) {
        case TERM_STATE_HYPERLINK_HEADER:
            if (byte == ';' && curr->cparamcount == 2) {
                if (curr->cparams[0] == '8' && curr->cparams[1] == ';') {
                    curr->state = TERM_STATE_HYPERLINK_URL;
                } else {
                    goto fail;
                }
            } else if (curr->cparamcount < TERMINAL_MAX_PARAMS) {
                curr->cparams[curr->cparamcount++] = byte;
            } else {
                goto fail;
            }
            break;

        case TERM_STATE_HYPERLINK_URL:
            if (byte == OSC_START) {
                curr->state = TERM_STATE_HYPERLINK_TEXT;
            }
            /* Skip URL processing for now */
            terminal_execute_osc_command(curr);
            break;

        case TERM_STATE_HYPERLINK_TEXT:
            if (byte == ESC_START) {
                curr->cparamcount = 0;
                curr->state = TERM_STATE_HYPERLINK_TAIL;
            }
            /* Skip text processing for now */
            terminal_execute_osc_command(curr);
            break;

        case TERM_STATE_HYPERLINK_TAIL:
            if (byte == OSC_START && curr->cparamcount == 4) {
                if (curr->cparams[0] == ']' && curr->cparams[1] == '8' &&
                    curr->cparams[2] == ';' && curr->cparams[3] == ';') {
                    goto success;
                } else {
                    goto fail;
                }
            } else if (curr->cparamcount < 4) {
                curr->cparams[curr->cparamcount++] = byte;
            } else {
                goto fail;
            }
            break;

        default:
            goto fail;
    }

    return SYS_OK;

success:
    curr->state = TERM_STATE_IDLE;
    curr->cparamcount = 0;
    terminal_stats_update(curr, TERM_OP_ESCAPE_SEQ);
    return SYS_OK;

fail:
    curr->state = TERM_STATE_IDLE;
    curr->cparamcount = 0;
    terminal_stats_update(curr, TERM_OP_ERROR);
    return SYS_ERR;
}

/**
 * @brief Executes a parsed OSC (Operating System Command).
 *
 * @param curr Pointer to the TERMINAL structure.
 * @return SYS_OK if execution was successful, SYS_ERR otherwise.
 */
static STATUS terminal_execute_osc_command(TERMINAL *curr) {
    if (!curr) return SYS_ERR;

    /* TODO: Implement OSC command execution */
    klogw("TERM execute_osc_command: Not currently supported\n");
    curr->state = TERM_STATE_IDLE;
    terminal_stats_update(curr, TERM_OP_ESCAPE_SEQ);
    return SYS_OK;
}

/**
 * @brief Erases the current line based on the given mode.
 *
 * @param curr Pointer to the TERMINAL structure.
 * @param mode Erase mode (0: to end, 1: from start, 2: entire line).
 * @return SYS_OK if successful, SYS_ERR on error or unknown mode.
 */
static STATUS terminal_erase_line(TERMINAL *curr, int mode) {
    if (!curr) return SYS_ERR;

    int fh = font.header->character_size;
    int fw = PSF1_FONT_WIDTH;
    int line_start = curr->cursor_pos.y * fh;
    int line_end = MIN((curr->cursor_pos.y + 1) * fh, (int)curr->framebuffer.height);

    for (int y = line_start; y < line_end; y++) {
        for (int x = 0; x < (int)curr->framebuffer.width; x++) {
            uint8_t should_clear = FALSE;

            switch (mode) {
                case 0:  /* Clear from cursor to end of line */
                    should_clear = (x >= curr->cursor_pos.x * fw);
                    break;
                case 1:  /* Clear from beginning of line to cursor */
                    should_clear = (x <= curr->cursor_pos.x * fw);
                    break;
                case 2:  /* Clear entire line */
                    should_clear = TRUE;
                    break;
                default:
                    return SYS_ERR;
            }

            if (should_clear) {
                fb_putpixel(&(curr->framebuffer), x, y, curr->background_color);
            }
        }
    }

    return SYS_OK;
}

/**
 * @brief Erases the display based on the given mode.
 *
 * @param curr Pointer to the TERMINAL structure.
 * @param mode Erase mode (0: to end, 1: from start, 2: entire screen).
 * @return SYS_OK if successful, SYS_ERR on error or unknown mode.
 */
static STATUS terminal_erase_display(TERMINAL *curr, int mode) {
    if (!curr) return SYS_ERR;

    int fh = font.header->character_size;
    int fw = PSF1_FONT_WIDTH;
    int cursor_pixel_x = curr->cursor_pos.x * fw;
    int cursor_pixel_y = curr->cursor_pos.y * fh;

    for (int y = 0; y < (int)curr->framebuffer.height; y++) {
        for (int x = 0; x < (int)curr->framebuffer.width; x++) {
            uint8_t should_clear = FALSE;

            switch (mode) {
                case 0:  /* Clear from cursor to end of screen */
                    should_clear = (y > cursor_pixel_y) ||
                                   (y >= cursor_pixel_y &&
                                    y < cursor_pixel_y + fh &&
                                    x >= cursor_pixel_x);
                    break;
                case 1:  /* Clear from beginning of screen to cursor */
                    should_clear = (y < cursor_pixel_y) ||
                                   (y >= cursor_pixel_y &&
                                    y < cursor_pixel_y + fh &&
                                    x <= cursor_pixel_x);
                    break;
                case 2:  /* Clear entire screen */
                    should_clear = TRUE;
                    break;
                default:
                    return SYS_ERR;
            }

            if (should_clear) {
                fb_putpixel(&(curr->framebuffer), x, y, curr->background_color);
            }
        }
    }

    if (mode == 2) {
        curr->cursor_pos = IVEC2_ZERO;
    }

    return SYS_OK;
}

/**
 * @brief Resets the terminal's internal state to default values.
 *
 * @param curr Pointer to the TERMINAL structure.
 */
static void terminal_reset_state(TERMINAL *curr) {
    if (!curr) return;

    curr->foreground_color = DEFAULT_FG;
    curr->background_color = DEFAULT_BG;
    curr->is_bold = FALSE;
    curr->is_underline = FALSE;
    curr->is_reverse = FALSE;
    curr->cursor_pos = IVEC2_ZERO;
    curr->saved_cursor_pos = IVEC2_ZERO;
    curr->state = TERM_STATE_IDLE;
    curr->auto_wrap = TRUE;
    curr->cursor_visible = TRUE;
    curr->insert_mode = FALSE;
    curr->origin_mode = FALSE;
    curr->scroll_top = 0;
    curr->scroll_bottom = curr->height - 1;
    curr->tab_width = TERMINAL_DEFAULT_TAB_WIDTH;
    curr->charset = TERM_CHARSET_ASCII;
    curr->cparamcount = 0;
    curr->last_qu_char = FALSE;
    curr->skip_left = 0;
}

/**
 * @brief Returns a pointer to the TERMINAL instance corresponding to a mode.
 *
 * @param mode Terminal mode (TERM_MODE_INFO or TERM_MODE_TERM).
 * @return Pointer to the terminal, or NULL on invalid mode.
 */
TERMINAL *terminal_get_by_mode(TERM_MODE mode) {
    switch (mode) {
        case TERM_MODE_INFO:
            return &term_info;
        case TERM_MODE_TERM:
            return &term_cli;
        default:
            return NULL;
    }
}

/**
 * @brief Outputs a single character to the terminal, with escape and UTF-8 handling.
 *
 * @param mode Terminal mode to write to.
 * @param c Character to output.
 */
void terminal_putc(TERM_MODE mode, uint8_t c) {
    TERMINAL *curr = terminal_get_by_mode(mode);
    if (!curr) {
        kloge("TERMINAL PUTC: Invalid terminal mode %d\n", mode);
        return;
    }

    if (terminal_validate_params(curr) != SYS_OK) {
        return;
    }

    /* Handle UTF-8 sequences */
    if ((c & 0x80) && curr->skip_left == 0 && curr->state == TERM_STATE_IDLE) {
        if ((c & 0xF0) == 0xF0) {
            curr->skip_left = 3;  /* 4-byte sequence */
        } else if ((c & 0xE0) == 0xE0) {
            curr->skip_left = 2;  /* 3-byte sequence */
        } else if ((c & 0xC0) == 0xC0) {
            curr->skip_left = 1;  /* 2-byte sequence */
        }
        return;
    } else if (curr->skip_left > 0) {
        curr->skip_left--;
        return;
    }

    /* Handle escape sequences */
    if (curr->last_qu_char && c != CSI_BRACKET && c != OSC_BRACKET) {
        curr->state = TERM_STATE_IDLE;
        curr->last_qu_char = FALSE;
        terminal_print(mode, ESC_START);  /* Print the escape character */
        terminal_putc(mode, c);           /* Reprocess current character */
        return;
    }

    curr->last_qu_char = FALSE;

    /* Try to parse as command first */
    if (terminal_parse_cmd(curr, c) == SYS_OK) {
        return;
    }

    /* If not a command, print as regular character */
    terminal_print(mode, c);
}

/**
 * @brief Outputs a null-terminated string to the terminal.
 *
 * @param mode Terminal mode to write to.
 * @param s String to output.
 */
void terminal_puts(TERM_MODE mode, const char *s) {
    if (!s) return;

    for (size_t i = 0; s[i]; i++) {
        terminal_putc(mode, s[i]);
    }
}

/**
 * @brief Scrolls the terminal display up by one line within the scroll region.
 *
 * @param t Pointer to the TERMINAL structure.
 */
void terminal_scroll(TERMINAL *t) {
    if (!t) {
        kloge("TERM SCROLL: Null terminal!\n");
        return;
    }

    if (terminal_validate_params(t) != SYS_OK) {
        return;
    }

    const uint8_t char_size = font.header->character_size;
    int scroll_lines = 1;  /* Number of lines to scroll */

    /* Calculate scroll region */
    int scroll_start_pixel = t->scroll_top * char_size;
    int scroll_end_pixel = (t->scroll_bottom + 1) * char_size;

    /* Scroll up by moving pixels */
    for (int y = scroll_start_pixel; y < scroll_end_pixel - (scroll_lines * char_size); y++) {
        for (size_t x = 0; x < t->framebuffer.width; x++) {
            uint32_t pixel = fb_getpixel(&(t->framebuffer), x,
                                         y + (scroll_lines * char_size));
            fb_putpixel(&(t->framebuffer), x, y, pixel);
        }
    }

    /* Clear the bottom lines */
    for (int y = scroll_end_pixel - (scroll_lines * char_size); y < scroll_end_pixel; y++) {
        for (size_t x = 0; x < t->framebuffer.width; x++) {
            fb_putpixel(&(t->framebuffer), x, y, t->background_color);
        }
    }

    /* Update statistics */
    t->stats.scroll_operations++;
}

/**
 * @brief Sets the foreground color of the terminal text.
 *
 * @param mode Terminal mode to update.
 * @param color New foreground color.
 */
void terminal_set_foreground(TERM_MODE mode, FRAMEBUFFER_COLORS color) {
    TERMINAL *curr = terminal_get_by_mode(mode);
    if (curr) {
        LOCK_LOCK(&term_lock);
        curr->foreground_color = color;
        UNLOCK_LOCK(&term_lock);
    }
}

/**
 * @brief Sets the background color of the terminal text.
 *
 * @param mode Terminal mode to update.
 * @param color New background color.
 */
void terminal_set_background(TERM_MODE mode, FRAMEBUFFER_COLORS color) {
    TERMINAL *curr = terminal_get_by_mode(mode);
    if (curr) {
        LOCK_LOCK(&term_lock);
        curr->background_color = color;
        UNLOCK_LOCK(&term_lock);
    }
}

/**
 * @brief Sets the terminal cursor position, handling wrapping and clamping.
 *
 * @param t Pointer to the TERMINAL structure.
 * @param x New x-coordinate of the cursor.
 * @param y New y-coordinate of the cursor.
 */
void set_terminal_cursor_pos(TERMINAL *t, uint32_t x, uint32_t y) {
    if (!t) {
        kloge("SET TERMINAL CURSOR POS: Null terminal!\n");
        return;
    }

    LOCK_LOCK(&term_lock);

    /* Handle line wrap */
    if (x >= t->width) {
        y += x / t->width;
        x = x % t->width;
    }

    /* Clamp to valid range */
    t->cursor_pos.x = MIN(x, t->width - 1);
    t->cursor_pos.y = MIN(y, t->height - 1);

    UNLOCK_LOCK(&term_lock);
}

/**
 * @brief Advances the terminal cursor by one position, wrapping and
 *        scrolling if needed.
 *
 * @param t Pointer to the TERMINAL structure.
 */
void terminal_push_cursor(TERMINAL *t) {
    if (!t) {
        kloge("TERMINAL PUSH CURSOR: Null terminal!\n");
        return;
    }

    LOCK_LOCK(&term_lock);

    t->cursor_pos.x++;

    if (t->cursor_pos.x >= (int)t->width) {
        if (t->auto_wrap) {
            t->cursor_pos.x = 0;
            t->cursor_pos.y++;

            if (t->cursor_pos.y >= (int)t->height) {
                terminal_scroll(t);
                t->cursor_pos.y--;
            }
        } else {
            t->cursor_pos.x = t->width - 1;
        }
    }

    UNLOCK_LOCK(&term_lock);
}

/**
 * @brief Moves the terminal cursor backward by one position.
 *
 * @param t Pointer to the TERMINAL structure.
 */
void terminal_pop_cursor(TERMINAL *t) {
    if (!t) {
        kloge("TERMINAL POP CURSOR: Null terminal!\n");
        return;
    }

    LOCK_LOCK(&term_lock);

    if (t->cursor_pos.x > 0) {
        t->cursor_pos.x--;
    } else if (t->cursor_pos.y > 0) {
        t->cursor_pos.y--;
        t->cursor_pos.x = t->width - 1;
    }

    UNLOCK_LOCK(&term_lock);
}

/**
 * @brief Sets the cursor glyph (character used for cursor rendering).
 *
 * @param c Character to use as the terminal cursor.
 */
void terminal_set_cursor(uint8_t c) {
    LOCK_LOCK(&term_lock);
    term_cursor = c;
    UNLOCK_LOCK(&term_lock);
}

/**
 * @brief Returns the currently active terminal mode.
 *
 * @return The current TERM_MODE.
 */
TERM_MODE terminal_get_mode(void) {
    return term_mode;
}

/**
 * @brief Sets the currently active terminal mode.
 *
 * @param mode The terminal mode to set (TERM_MODE_INFO or TERM_MODE_TERM).
 */
void terminal_set_current_mode(TERM_MODE mode) {
    if (mode == TERM_MODE_INFO || mode == TERM_MODE_TERM) {
        LOCK_LOCK(&term_lock);
        term_mode = mode;
        term_need_redrawn = TRUE;
        UNLOCK_LOCK(&term_lock);
    }
}

/* --------------------------- WINSIZE FUNCTIONALITY ------------------------- */

/**
 * @brief Retrieves the current window (terminal) size.
 *
 * @param ws Pointer to WINDOW_SIZE structure to populate.
 * @return SYS_OK if successful, SYS_ERR on failure.
 */
STATUS terminal_get_winsize(WINDOW_SIZE *ws) {
    if (!ws) {
        return SYS_ERR;
    }

    TERMINAL *curr = &term_cli;  /* Use CLI terminal for window size */

    if (terminal_validate_params(curr) != SYS_OK) {
        return SYS_ERR;
    }

    LOCK_LOCK(&term_lock);

    ws->col = curr->width;
    ws->row = curr->height;
    ws->xpixel = curr->framebuffer.width;
    ws->ypixel = curr->framebuffer.height;

    UNLOCK_LOCK(&term_lock);

    return SYS_OK;
}

/**
 * @brief Attempts to set the window (terminal) size.
 *
 * @param ws Pointer to WINDOW_SIZE structure containing the requested size.
 * @return SYS_OK if unchanged or valid, SYS_ERR if resizing is unsupported or failed.
 */
STATUS terminal_set_winsize(WINDOW_SIZE *ws) {
    if (!ws) {
        return SYS_ERR;
    }

    TERMINAL *curr = &term_cli;  /* Use CLI terminal for window size */

    if (terminal_validate_params(curr) != SYS_OK) {
        return SYS_ERR;
    }

    LOCK_LOCK(&term_lock);

    /* Check if the requested size is different from current */
    if (ws->col != curr->width || ws->row != curr->height ||
        ws->xpixel != curr->framebuffer.width ||
        ws->ypixel != curr->framebuffer.height) {

        /* For now, we don't support dynamic resizing */
        kloge("TERMINAL SET WINSIZE: Dynamic resizing not supported\n");
        klogi("TERMINAL SET WINSIZE: Requested: %dx%d chars (%dx%d px)\n",
              ws->col, ws->row, ws->xpixel, ws->ypixel);
        klogi("TERMINAL SET WINSIZE: Current:   %dx%d chars (%dx%d px)\n",
              curr->width, curr->height, curr->framebuffer.width, curr->framebuffer.height);

        UNLOCK_LOCK(&term_lock);
        return SYS_ERR;
    }

    UNLOCK_LOCK(&term_lock);
    return SYS_OK;
}

/* --------------------------- STATISTICS AND DEBUGGING ---------------------- */

/**
 * @brief Retrieve terminal statistics for a specific terminal mode.
 *
 * @param mode Terminal mode to query (e.g., TERM_MODE_TERM).
 * @param stats Pointer to a TERMINAL_STATS struct to populate.
 * @return SYS_OK if successful, SYS_ERR on error or invalid pointer.
 */
STATUS terminal_get_stats(TERM_MODE mode, TERMINAL_STATS *stats) {
    if (!stats) return SYS_ERR;

    TERMINAL *curr = terminal_get_by_mode(mode);
    if (!curr) return SYS_ERR;

    LOCK_LOCK(&term_lock);
    *stats = curr->stats;
    UNLOCK_LOCK(&term_lock);

    return SYS_OK;
}

/**
 * @brief Retrieve global terminal statistics (aggregated).
 *
 * @param stats Pointer to a TERMINAL_STATS struct to populate.
 * @return SYS_OK if successful, SYS_ERR on error or invalid pointer.
 */
STATUS terminal_get_global_stats(TERMINAL_STATS *stats) {
    if (!stats) return SYS_ERR;

    LOCK_LOCK(&term_lock);
    *stats = global_stats;
    UNLOCK_LOCK(&term_lock);

    return SYS_OK;
}

/**
 * @brief Reset terminal statistics for a specific terminal mode.
 *
 * @param mode Terminal mode whose stats should be reset.
 */
void terminal_reset_stats(TERM_MODE mode) {
    TERMINAL *curr = terminal_get_by_mode(mode);
    if (!curr) return;

    LOCK_LOCK(&term_lock);
    memset(&curr->stats, 0, sizeof(TERMINAL_STATS));
    UNLOCK_LOCK(&term_lock);
}

/**
 * @brief Reset terminal statistics for a specific terminal mode.
 *
 * @param mode Terminal mode whose stats should be reset.
 */
void terminal_reset_global_stats(void) {
    LOCK_LOCK(&term_lock);
    memset(&global_stats, 0, sizeof(TERMINAL_STATS));
    UNLOCK_LOCK(&term_lock);
}

/**
 * @brief Get information about the terminal state for debugging.
 *
 * @param mode Terminal mode to query (e.g., TERM_MODE_TERM).
 * @param buffer Buffer to store formatted information string.
 * @param buffer_size Size of the buffer.
 * @return SYS_OK if successfully written, SYS_ERR on error or overflow.
 */
STATUS terminal_get_info(TERM_MODE mode, char *buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return SYS_ERR;

    TERMINAL *curr = terminal_get_by_mode(mode);
    if (!curr) return SYS_ERR;

    LOCK_LOCK(&term_lock);

    int written = snprintf(buffer, buffer_size,
        "Terminal Info (%s):\n"
        "  Dimensions: %dx%d chars (%dx%d px)\n"
        "  Cursor: (%d,%d)\n"
        "  Colors: FG=%8x, BG=0%8x\n"
        "  State: %d, Bold: %s, Wrap: %s\n"
        "  Scroll Region: %d-%d\n"
        "  Stats: %d chars, %d lines, %d scrolls, %d errors\n",
        (mode == TERM_MODE_INFO) ? "INFO" : "CLI",
        curr->width, curr->height,
        curr->framebuffer.width, curr->framebuffer.height,
        curr->cursor_pos.x, curr->cursor_pos.y,
        curr->foreground_color, curr->background_color,
        curr->state, curr->is_bold ? "Yes" : "No", curr->auto_wrap ? "Yes" : "No",
        curr->scroll_top, curr->scroll_bottom,
        curr->stats.chars_written, curr->stats.lines_written,
        curr->stats.scroll_operations, curr->stats.errors);

    UNLOCK_LOCK(&term_lock);

    return (written > 0 && written < (int)buffer_size) ? SYS_OK : SYS_ERR;
}

/**
 * @brief Initialize terminal subsystem with a given framebuffer.
 *
 * @param fb Pointer to a limine_framebuffer structure.
 */
void init_terminal(struct limine_framebuffer *fb) {
    klogs("INIT TERMINAL: starting...\n");

    /* Validate font */
    if (!font.header) {
        kloge("INIT TERMINAL: Font header is NULL!\n");
        return;
    }

    /* Initialize both terminals */
    TERMINAL *terminals[] = {&term_info, &term_cli};
    const char *names[] = {"INFO", "CLI"};
    TERM_MODE modes[] = {TERM_MODE_INFO, TERM_MODE_TERM};

    for (int i = 0; i < 2; i++) {
        TERMINAL *curr = terminals[i];

        /* Initialize framebuffer */
        if (fb_init(&(curr->framebuffer), fb) == SYS_ERR) {
            kloge("TERM init_terminal: framebuffer failed to init\n");
            continue;
        }

        /* Calculate terminal dimensions */
        curr->width = curr->framebuffer.width / PSF1_FONT_WIDTH;
        curr->height = curr->framebuffer.height / font.header->character_size;

        /* Set default colors and state */
        curr->foreground_color = DEFAULT_FG;
        curr->background_color = DEFAULT_BG;
        curr->state = TERM_STATE_IDLE;
        curr->cursor_pos = IVEC2_ZERO;
        curr->last_char = 0;
        curr->last_qu_char = FALSE;
        curr->skip_left = 0;
        curr->mode = modes[i];
        curr->is_bold = FALSE;
        curr->cparamcount = 0;

        /* Initialize enhanced features */
        curr->auto_wrap = TRUE;
        curr->cursor_visible = TRUE;
        curr->insert_mode = FALSE;
        curr->origin_mode = FALSE;
        curr->charset = TERM_CHARSET_ASCII;
        curr->tab_width = TERMINAL_DEFAULT_TAB_WIDTH;

        /* Set up termios */
        curr->termios = global_termios;
        curr->termios_enabled = TRUE;

        /* Initialize scroll region */
        curr->scroll_top = 0;
        curr->scroll_bottom = curr->height - 1;

        /* Initialize statistics */
        memset(&curr->stats, 0, sizeof(TERMINAL_STATS));

        /* Clear and refresh terminal */
        terminal_clear(modes[i]);
        terminal_refresh(modes[i]);

        klogi("INIT TERMINAL: Initialized %s terminal\n", names[i]);
        klogt("\tDimensions:   %dx%d chars (%dx%d px)\n",
              curr->width, curr->height,
              curr->framebuffer.width, curr->framebuffer.height);
        klogt("\tPitch:        %d bytes\n", curr->framebuffer.pitch);
        klogt("\tBase Address: %x\n", curr->framebuffer.base);
    }

    /* Set default character printing */
    term_char_print = DEFAULT_CHAR_PRINT;

    klogs("INIT TERMINAL: finished...\n");
}

/**
 * @brief Start terminal operation and set the initial mode.
 */
void terminal_start(void) {
    /* Reinitialize framebuffers if needed */
    fb_init(&(term_info.framebuffer), NULL);
    fb_init(&(term_cli.framebuffer), NULL);

    /* Refresh kernel logs */
    klog_refresh(TERM_MODE_TERM);
    klog_refresh(TERM_MODE_INFO);

    /* Set initial mode based on configuration */
    #if CLI
    terminal_clear((term_mode = TERM_MODE_TERM));
    terminal_refresh(term_mode);
    #else
    term_mode = TERM_MODE_INFO;
    #endif

    term_need_redrawn = TRUE;
}

/**
 * @brief Enable character printing to the framebuffer.
 */
void terminal_enable_character_printing(void) {
    LOCK_LOCK(&term_lock);
    term_char_print = 1;
    UNLOCK_LOCK(&term_lock);
}

/**
 * @brief Disable character printing to the framebuffer.
 */
void terminal_disable_character_printing(void) {
    LOCK_LOCK(&term_lock);
    term_char_print = 0;
    UNLOCK_LOCK(&term_lock);
}

/**
 * @brief Check if the terminal needs to be redrawn.
 *
 * @return 1 if redraw is needed, 0 otherwise.
 */
uint8_t terminal_need_redraw(void) {
    return term_need_redrawn;
}

/**
 * @brief Set the terminal's redraw flag.
 *
 * @param flag 1 to enable redraw, 0 to disable.
 */
void terminal_set_redraw(uint8_t flag) {
    LOCK_LOCK(&term_lock);
    term_need_redrawn = flag;
    UNLOCK_LOCK(&term_lock);
}

/**
 * @brief Clear the terminal screen and reset internal state.
 *
 * @param mode Terminal mode to clear.
 */
void terminal_clear(TERM_MODE mode) {
    TERMINAL *curr = terminal_get_by_mode(mode);
    if (!curr) {
        kloge("TERMINAL CLEAR: Invalid terminal mode %d\n", mode);
        return;
    }

    if (terminal_validate_params(curr) != SYS_OK) {
        kloge("TERMINAL CLEAR: Terminal validation failed\n");
        return;
    }

    LOCK_LOCK(&term_lock);

    /* Copy background buffer if available */
    if (curr->framebuffer.background_buffer) {
        memcpy(curr->framebuffer.base, curr->framebuffer.background_buffer,
               curr->framebuffer.width * curr->framebuffer.height * 4);
    }

    /* Clear entire screen */
    for (size_t y = 0; y < curr->framebuffer.height; y++) {
        for (size_t x = 0; x < curr->framebuffer.width; x++) {
            fb_putpixel(&(curr->framebuffer), x, y, curr->background_color);
        }
    }

    /* Reset cursor position and state */
    curr->cursor_pos = IVEC2_ZERO;
    curr->state = TERM_STATE_IDLE;
    curr->last_qu_char = FALSE;
    curr->skip_left = 0;
    curr->cparamcount = 0;

    /* Update statistics */
    curr->stats.clear_operations++;
    global_stats.clear_operations++;

    UNLOCK_LOCK(&term_lock);
}

/**
 * @brief Refresh terminal display and update cursor if necessary.
 *
 * @param mode Terminal mode to refresh.
 * @return SYS_OK if successful, SYS_ERR on failure.
 */
STATUS terminal_refresh(TERM_MODE mode) {
    TERMINAL *curr = terminal_get_by_mode(mode);
    if (!curr) {
        return SYS_ERR;
    }

    if (terminal_validate_params(curr) != SYS_OK) {
        return SYS_ERR;
    }

    LOCK_LOCK(&term_lock);

    /* Handle cursor display for terminal mode */
    if (mode == TERM_MODE_TERM && term_cursor != 0) {
        terminal_update_cursor_visibility(curr, mode);

        uint32_t x = curr->cursor_pos.x;
        uint32_t y = curr->cursor_pos.y;

        if (x < curr->width && y < curr->height) {
            fb_putc(&(curr->framebuffer), x * PSF1_FONT_WIDTH,
                    y * font.header->character_size, curr->foreground_color,
                    curr->background_color, term_cursor, curr->is_bold);
        }
    }

    /* Refresh framebuffer if this is the active mode */
    if (mode == term_mode) {
        fb_refresh(&(curr->framebuffer));
    }

    UNLOCK_LOCK(&term_lock);
    return SYS_OK;
}

/**
 * @brief Print a single character to the terminal.
 *
 * @param mode Terminal mode to print in.
 * @param c Character to print.
 */
void terminal_print(TERM_MODE mode, uint8_t c) {
    TERMINAL *curr = terminal_get_by_mode(mode);
    if (!curr) {
        kloge("TERMINAL PRINT: Invalid terminal mode %d\n", mode);
        return;
    }

    #if CLI
    /* Send to serial port */
    serial_write(c);

    /* Skip info mode output if not currently active */
    if (mode == TERM_MODE_INFO && terminal_get_mode() != TERM_MODE_INFO) {
        return;
    }
    #endif

    if (!term_char_print || terminal_validate_params(curr) != SYS_OK) {
        return;
    }

    LOCK_LOCK(&term_lock);

    /* Handle control characters */
    if (c < 0x20) {
        if (terminal_handle_control_char(curr, c) == SYS_OK) {
            UNLOCK_LOCK(&term_lock);
            return;
        }
    }

    /* Handle scrolling if at bottom of screen */
    if (curr->cursor_pos.y >= (int)curr->height) {
        terminal_scroll(curr);
        curr->cursor_pos.y--;
        terminal_stats_update(curr, TERM_OP_SCROLL);
    }

    /* Handle printable characters */
    if (c >= 0x20 && c <= 0x7E) {
        /* Standard ASCII printable characters */

        /* Check for line wrap */
        if (curr->cursor_pos.x >= (int)curr->width) {
            if (curr->auto_wrap) {
                curr->cursor_pos.x = 0;
                curr->cursor_pos.y++;
                terminal_resize_check(curr);
            } else {
                curr->cursor_pos.x = curr->width - 1;
            }
        }

        /* Display character */
        fb_putc(&(curr->framebuffer),
                curr->cursor_pos.x * PSF1_FONT_WIDTH,
                curr->cursor_pos.y * font.header->character_size,
                curr->foreground_color, curr->background_color,
                c, curr->is_bold);

        /* Advance cursor */
        curr->cursor_pos.x++;
        terminal_stats_update(curr, TERM_OP_CHAR_WRITE);

    } else if (c > 0x7E) {
        /* Extended ASCII or Unicode handling */
        if (c <= 0xA0 || curr->last_char == 0) {
            /* Single-byte extended character */
            if (curr->cursor_pos.x >= (int)curr->width) {
                if (curr->auto_wrap) {
                    curr->cursor_pos.x = 0;
                    curr->cursor_pos.y++;
                    terminal_resize_check(curr);
                }
            }

            fb_putc(&(curr->framebuffer),
                    curr->cursor_pos.x * PSF1_FONT_WIDTH,
                    curr->cursor_pos.y * font.header->character_size,
                    curr->foreground_color, curr->background_color,
                    c, curr->is_bold);

            curr->cursor_pos.x++;
            terminal_stats_update(curr, TERM_OP_CHAR_WRITE);
        } else {
            /* Multi-byte character (display as placeholder) */
            fb_putc(&(curr->framebuffer),
                    curr->cursor_pos.x * PSF1_FONT_WIDTH,
                    curr->cursor_pos.y * font.header->character_size,
                    curr->foreground_color, curr->background_color,
                    '?', FALSE);
            curr->cursor_pos.x++;
            terminal_stats_update(curr, TERM_OP_CHAR_WRITE);
        }
    }

    /* Ensure cursor position is valid */
    terminal_resize_check(curr);

    UNLOCK_LOCK(&term_lock);
}

/**
 * @brief Parse ANSI escape and control sequences for the terminal.
 *
 * @param curr Pointer to the TERMINAL structure.
 * @param byte Input byte to parse.
 * @return SYS_OK if successfully handled, SYS_ERR otherwise.
 */
STATUS terminal_parse_cmd(TERMINAL *curr, uint8_t byte) {
    if (!curr) {
        kloge("TERMINAL PARSE CMD: Null terminal!\n");
        return SYS_ERR;
    }

    if (curr->state == TERM_STATE_UNKNOWN) {
        return SYS_ERR;
    }

    /* Handle multi-byte UTF-8 sequences */
    if ((byte & 0x80) && curr->skip_left == 0 && curr->state == TERM_STATE_IDLE) {
        if ((byte & 0xF0) == 0xF0) {
            curr->skip_left = 3;  /* 4-byte sequence */
        } else if ((byte & 0xE0) == 0xE0) {
            curr->skip_left = 2;  /* 3-byte sequence */
        } else if ((byte & 0xC0) == 0xC0) {
            curr->skip_left = 1;  /* 2-byte sequence */
        }
        return SYS_OK;
    } else if (curr->skip_left > 0) {
        curr->skip_left--;
        return SYS_OK;
    }

    /* Handle escape sequence states */
    switch (curr->state) {
        case TERM_STATE_IDLE:
            if (byte == ESC_START) {
                curr->state = TERM_STATE_CMD;
                curr->last_qu_char = TRUE;
                return SYS_OK;
            }
            break;

        case TERM_STATE_CMD:
            switch (byte) {
                case CSI_BRACKET:  /* CSI sequence */
                    curr->cparamcount = 1;
                    curr->cparams[0] = 0;
                    curr->state = TERM_STATE_PARAM;
                    return SYS_OK;

                case OSC_BRACKET:  /* OSC sequence */
                    curr->cparamcount = 0;
                    curr->state = TERM_STATE_HYPERLINK_HEADER;
                    curr->last_qu_char = FALSE;
                    return SYS_OK;

                /* Single-character escape sequences */
                case 'c':  /* Reset terminal */
                    terminal_reset_state(curr);
                    goto success;

                case 'D':  /* Line feed */
                    curr->cursor_pos.y++;
                    terminal_resize_check(curr);
                    goto success;

                case 'E':  /* New line */
                    curr->cursor_pos.x = 0;
                    curr->cursor_pos.y++;
                    terminal_resize_check(curr);
                    goto success;

                case 'M':  /* Reverse line feed */
                    if (curr->cursor_pos.y > curr->scroll_top) {
                        curr->cursor_pos.y--;
                    }
                    goto success;

                default:
                    goto fail;
            }
            break;

        case TERM_STATE_PARAM:
            return terminal_parse_csi_sequence(curr, byte);

        case TERM_STATE_HYPERLINK_HEADER:
        case TERM_STATE_HYPERLINK_URL:
        case TERM_STATE_HYPERLINK_TEXT:
        case TERM_STATE_HYPERLINK_TAIL:
            return terminal_parse_osc_sequence(curr, byte);

        default:
            goto fail;
    }

    return SYS_ERR;

success:
    curr->state = TERM_STATE_IDLE;
    curr->cparamcount = 0;
    curr->last_qu_char = FALSE;
    terminal_stats_update(curr, TERM_OP_ESCAPE_SEQ);
    return SYS_OK;

fail:
    curr->state = TERM_STATE_IDLE;
    curr->cparamcount = 0;
    terminal_stats_update(curr, TERM_OP_ERROR);
    return SYS_ERR;
}

/**
 * @brief Handle terminal I/O control operations
 *
 * @param request IOCTL request code
 * @param arg Pointer to argument data
 * @return int64_t 0 on success, -1 on failure
 */
int64_t terminal_ioctl(int64_t request, int64_t arg) {
    TERMINAL *curr = terminal_get_by_mode(TERM_MODE_TERM);
    if (!curr) {
        return -1;
    }

    LOCK_LOCK(&term_lock);
    int64_t ret = 0;

    switch (request) {
        case TCGETS: {
            /* Get terminal attributes */
            TERMIOS *t = (TERMIOS *)arg;
            if (!t) {
                ret = -1;
                break;
            }
            *t = curr->termios;
            klogi("TERMINAL IOCTL: TCGETS - returning termios settings\n");
            break;
        }

        case TCSETS:
        case TCSETSW:
        case TCSETSF: {
            /* Set terminal attributes */
            TERMIOS *t = (TERMIOS *)arg;
            if (!t) {
                ret = -1;
                break;
            }

            /* Store the new settings */
            curr->termios = *t;

            /* Apply settings that affect terminal behavior */
            if (t->c_lflag & ECHO) {
                klogi("TERMINAL IOCTL: Echo enabled\n");
            } else {
                klogi("TERMINAL IOCTL: Echo disabled\n");
            }

            if (t->c_lflag & ICANON) {
                klogi("TERMINAL IOCTL: Canonical mode enabled\n");
            } else {
                klogi("TERMINAL IOCTL: Raw mode enabled\n");
            }

            /* Handle TCSETSF - flush input if requested */
            if (request == TCSETSF) {
                klogi("TERMINAL IOCTL: TCSETSF - flushing will be handled by ttyfs\n");
            }

            klogi("TERMINAL IOCTL: %s - terminal attributes set\n",
                  (request == TCSETS) ? "TCSETS" :
                  (request == TCSETSW) ? "TCSETSW" : "TCSETSF");
            break;
        }

        case TCFLSH: {
            /* Flush input/output queues */
            int queue = (int)arg;
            switch (queue) {
                case TCIFLUSH:
                    klogi("TERMINAL IOCTL: TCFLSH - input flush (handled by ttyfs)\n");
                    break;
                case TCOFLUSH:
                    klogi("TERMINAL IOCTL: TCFLSH - output flush\n");
                    terminal_refresh(TERM_MODE_TERM);
                    break;
                case TCIOFLUSH:
                    klogi("TERMINAL IOCTL: TCFLSH - input/output flush\n");
                    terminal_refresh(TERM_MODE_TERM);
                    break;
                default:
                    ret = -1;
                    break;
            }
            break;
        }

        case TIOCGWINSZ: {
            /* Get window size */
            WINDOW_SIZE *ws = (WINDOW_SIZE *)arg;
            if (!ws) {
                ret = -1;
                break;
            }
            UNLOCK_LOCK(&term_lock);
            ret = (terminal_get_winsize(ws) == SYS_OK) ? 0 : -1;
            LOCK_LOCK(&term_lock);
            break;
        }

        case TIOCSWINSZ: {
            /* Set window size */
            WINDOW_SIZE *ws = (WINDOW_SIZE *)arg;
            if (!ws) {
                ret = -1;
                break;
            }
            UNLOCK_LOCK(&term_lock);
            ret = (terminal_set_winsize(ws) == SYS_OK) ? 0 : -1;
            LOCK_LOCK(&term_lock);
            break;
        }

        case TIOCGPGRP: {
            /* Get process group */
            klogw("TERMINAL IOCTL: TIOCGPGRP not implemented\n");
            ret = -1;
            break;
        }

        case TIOCSPGRP: {
            /* Set process group */
            klogw("TERMINAL IOCTL: TIOCSPGRP not implemented\n");
            ret = -1;
            break;
        }

        default:
            kloge("TERMINAL IOCTL: Unknown request %x\n", request);
            ret = -1;
            break;
    }

    UNLOCK_LOCK(&term_lock);
    return ret;
}