/**
 * @file terminal.c
 * @author Zack Bostock
 * @brief Helpers and initialization of the terminal
 * @verbatim
 *
 * @copyright Copyright (c) 2024
 *
 */


#include <kconfig.h>

#include <common/lock.h>
#include <common/kprint.h>
#include <common/math.h>
#include <common/memory.h>

#include <graphics/framebuffer.h>

#include <dev/terminal.h>
#include <dev/serial.h>

/**
 * @brief Basic 4 bit VGA colors in correct order for escape sequence
 */
static const uint32_t four_bit_colors[16] = {
    COLOR_BLACK,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_BLUE,
    COLOR_MAGENTA,
    COLOR_CYAN,
    COLOR_WHITE,
    COLOR_BRIGHT_BLACK,
    COLOR_BRIGHT_RED,
    COLOR_BRIGHT_GREEN,
    COLOR_BRIGHT_YELLOW,
    COLOR_BRIGHT_BLUE,
    COLOR_BRIGHT_MAGENTA,
    COLOR_BRIGHT_CYAN,
    COLOR_BRIGHT_WHITE
};

static LOCK term_lock = {0};
static TERM_MODE term_mode = TERM_MODE_UNSET;
static uint8_t term_need_redrawn = FALSE;
static TERMINAL term_info = {0};
static TERMINAL term_cli = {0};

TERM_CURSOR_STATUS cursor_visible = 0;

/**
 * @brief Used for ANSI Escape Sequences
 */
#define CSI_START   '\033'
#define OSC_START   '\007'

/**
 * @brief Main initialization of a terminal
 * @verbatim
 * Initializes a framebuffer for the terminal and sets basic information about
 * the framebuffer and the terminal variable to support text output.
 *
 * @param fb Framebuffer to use with the terminals
 */
void init_terminal(struct limine_framebuffer *fb) {
    klogs("INIT TERMINAL: starting...\n");

    /* TODO: refactor for better font support */
    if (!font.header) {
        kloge("INIT TERMINAL: Trying to use a NULL font header!\n");
        return;
    }

    /* Set up information terminal */
    TERMINAL *curr;
    for (int i = 0; i < 2; i++) {
        curr = (i == 0) ? &term_info : &term_cli;
        fb_init(&(curr->framebuffer), fb);
        curr->width = curr->framebuffer.width / PSF1_FONT_WIDTH;
        curr->height = curr->framebuffer.height / font.header->character_size;
        curr->foreground_color = DEFAULT_FG;
        curr->background_color = DEFAULT_BG;
        curr->state = TERM_STATE_IDLE;
        curr->cursor_pos = IVEC2_ZERO;
        curr->last_char = 0;
        curr->mode = (i == 0) ? TERM_MODE_INFO : TERM_MODE_TERM;
        terminal_clear(curr->mode);
        terminal_refresh(curr->mode);
        klogd("INIT TERMINAL: New terminal (term %d)\n", i);
        klogt("\tWidth:        %d px\n", curr->framebuffer.width);
        klogt("\tHeight:       %d px\n", curr->framebuffer.height);
        klogt("\tPitch:        %d\n", curr->framebuffer.pitch);
        klogt("\tFB Base Addr: %x\n", curr->framebuffer.base);
    }

    klogs("INIT TERMINAL: finished...\n");
}

/**
 * @brief Function which starts the terminals and sets them up
 */
void terminal_start() {
    fb_init(&(term_info.framebuffer), NULL);
    fb_init(&(term_cli.framebuffer), NULL);

    klog_refresh(TERM_MODE_TERM);
    klog_refresh(TERM_MODE_INFO);

    #if CLI
    terminal_clear((term_mode = TERM_MODE_TERM));
    fb_draw_characters(&(term_cli.framebuffer), COLOR_BRIGHT_GREEN, DEFAULT_BG,
                       "Horizon64");
    terminal_refresh(term_mode);
    #else
    term_mode = TERM_MODE_INFO;
    #endif

    term_need_redrawn = TRUE;
}

/**
 * @brief Helper used to determine if the terminal needs redrawn
 *
 * @return uint8_t TRUE if yes, FALSE otherwise
 */
uint8_t terminal_need_redraw() {
    return term_need_redrawn;
}

/**
 * @brief Helper to change whether the terminal needs redrawn
 *
 * @param a TRUE or FALSE
 */
void terminal_set_redraw(uint8_t a) {
    term_need_redrawn = a;
}

/**
 * @brief Clears the terminal to a known state depending on the terminal mode
 *
 * @param mode Current mode of the terminal
 */
void terminal_clear(TERM_MODE mode) {
    TERMINAL *curr;
    if (mode == TERM_MODE_INFO) {
        curr = &term_info;
    } else if (mode == TERM_MODE_TERM) {
        curr = &term_cli;
    } else {
        kloge("TERMINAL CLEAR: Trying to clear a terminal which is unknown!\n");
        return;
    }

    if (curr->state == TERM_STATE_UNKNOWN) {
        kloge("TERMINAL CLEAR: Trying to clear a terminal whose state is unknown!\n");
        return;
    }

    if (curr->framebuffer.background_buffer) {
        memcpy(curr->framebuffer.base, curr->framebuffer.background_buffer,
               curr->framebuffer.width * curr->framebuffer.height * 4);
    }

    for (size_t y = 0; y < curr->framebuffer.height; y++) {
        for (size_t x = 0; x < curr->framebuffer.width; x++) {
            fb_putpixel(&(curr->framebuffer), x, y, curr->background_color);
        }
    }

    curr->cursor_pos = IVEC2_ZERO;
}

/**
 * @brief Refresh the terminal framebuffer
 *
 * @param mode Mode of the terminal
 * @return STATUS SYS_OK if success, SYS_ERR if failure
 */
STATUS terminal_refresh(TERM_MODE mode) {
    TERMINAL *curr;

    LOCK_LOCK(&term_lock);

    if (mode == TERM_MODE_INFO) {
        curr = &term_info;
    } else if (mode == TERM_MODE_TERM) {
        curr = &term_cli;

        if (cursor_visible != 0) {
            if (curr->state != TERM_STATE_UNKNOWN && mode == term_mode) {
                fb_refresh(&(curr->framebuffer));
            }

            uint32_t x = curr->cursor_pos.x;
            uint32_t y = curr->cursor_pos.y;
            if (x < curr->width && y < curr->height) {
                fb_putc(&(curr->framebuffer), x * PSF1_FONT_WIDTH,
                        y * font.header->character_size, curr->foreground_color,
                        curr->background_color, cursor_visible, curr->is_bold);
                if (mode == term_mode) {
                    fb_refresh(&(curr->framebuffer));
                }
                UNLOCK_LOCK(&term_lock);
                return SYS_OK;
            }
        }
    } else {
        kloge("TERMINAL REFRESH: Trying to refresh a terminal mode which isn't bound!\n");
        UNLOCK_LOCK(&term_lock);
        return SYS_ERR;
    }

    if (curr->state == TERM_STATE_UNKNOWN) {
        UNLOCK_LOCK(&term_lock);
        return SYS_ERR;
    }

    if (mode == term_mode) {
        fb_refresh(&(curr->framebuffer));
    }

    UNLOCK_LOCK(&term_lock);
    return SYS_OK;
}

void terminal_print(TERM_MODE mode, uint8_t c) {
    #ifdef CLI
    if (term_mode != TERM_MODE_UNSET) {
        serial_write(c);
    }

    if (mode == TERM_MODE_INFO && terminal_get_mode() != TERM_MODE_INFO) {
        return;
    }
    #endif

    TERMINAL *curr;

    if (mode == TERM_MODE_INFO) {
        curr = &term_info;
    } else if (mode == TERM_MODE_TERM) {
        curr = &term_cli;
    } else {
        kloge("TERMINAL PRINT: Trying to print to a terminal which is unknown!\n");
        return;
    }

    if (c == '\b') {
        terminal_print(mode, ' ');
        if (curr->cursor_pos.x > 0) {
            curr->cursor_pos.x--;
        }
        if (curr->cursor_pos.x > 0) {
            curr->cursor_pos.x--;
        }
        return;
    }

    if (curr->cursor_pos.y == (int)curr->height && c != '\0') {
        terminal_scroll(curr);
        curr->cursor_pos.y--;
    }

    switch (c) {
        case '\0':
            /* Nul byte */
            return;
        case '\n':
            /* New line character */
            cursor_visible = ' ';
            terminal_refresh(mode);
            curr->cursor_pos.x = 0;
            curr->cursor_pos.y++;
            break;
        case '\t':
            /* Tab character */
            curr->cursor_pos.x += (curr->cursor_pos.x % 8 == 0) ? 8 : (8 - curr->cursor_pos.x % 8);
            if (curr->cursor_pos.x > (int)curr->width) {
                curr->cursor_pos.x -= curr->width;
                curr->cursor_pos.y++;
            }
            break;
        default:
            /* Any other character */
            if (c <= 0xA0 || (c > 0xA0 && curr->last_char == 0)) {
                /* CRLF, if needed */
                if (curr->cursor_pos.x >= (int)curr->width) {
                    curr->cursor_pos.x = 0;
                    curr->cursor_pos.y++;
                }

                /* Check if we need to scroll the screen */
                if (curr->cursor_pos.y >= (int)curr->height) {
                    terminal_scroll(curr);
                    curr->cursor_pos.y--;
                }

                fb_putc(&(curr->framebuffer), curr->cursor_pos.x * PSF1_FONT_WIDTH,
                        curr->cursor_pos.y * font.header->character_size,
                        curr->foreground_color, curr->background_color,
                        c, curr->is_bold);

                /* Push cursor position */
                curr->cursor_pos.x++;
            } else {
                /* CRLF, if needed */
                if (curr->cursor_pos.x >= (int)(curr->width - 1)) {
                    curr->cursor_pos.x = 0;
                    curr->cursor_pos.y++;
                }

                /* Check if we need to scroll the screen */
                if (curr->cursor_pos.y >= (int)curr->height) {
                    terminal_scroll(curr);
                    curr->cursor_pos.y--;
                }

                curr->last_char = 0;

                /* Unknown character, print out question marks */
                fb_putc(&(curr->framebuffer), curr->cursor_pos.x * PSF1_FONT_WIDTH,
                        curr->cursor_pos.y * font.header->character_size,
                        curr->foreground_color, curr->background_color,
                        '?', FALSE);
                curr->cursor_pos.x++;
                fb_putc(&(curr->framebuffer), curr->cursor_pos.x * PSF1_FONT_WIDTH,
                        curr->cursor_pos.y * font.header->character_size,
                        curr->foreground_color, curr->background_color,
                        '?', FALSE);
                curr->cursor_pos.x++;
            }
            break;
    }

    /* Make sure the cursor is in the correct position */
    while (TRUE) {
        if (curr->cursor_pos.x >= (int)curr->width) {
            curr->cursor_pos.x = 0;
            curr->cursor_pos.y++;
        }

        if (curr->cursor_pos.y >= (int)curr->height &&
            !(curr->cursor_pos.y == (int)curr->height &&
            curr->cursor_pos.x == 0)) {
            terminal_scroll(curr);
            curr->cursor_pos.y--;
        } else {
            break;
        }
    }
}

/**
 * @brief Helper to set the foreground of the terminal
 *
 * @param mode Terminal mode which maps to a terminal to change
 * @param color Color to set the foreground to
 */
void terminal_set_foreground(TERM_MODE mode, FRAMEBUFFER_COLORS color) {
    if (mode == TERM_MODE_TERM) {
        term_cli.foreground_color = color;
    } else if (mode == TERM_MODE_INFO) {
        term_info.foreground_color = color;
    }
    return;
}

/**
 * @brief Helper for setting the background of the terminal
 *
 * @param mode Terminal mode which maps to a terminal to change
 * @param color Color to set the background to
 */
void terminal_set_background(TERM_MODE mode, FRAMEBUFFER_COLORS color) {
    if (mode == TERM_MODE_TERM) {
        term_cli.foreground_color = color;
    } else if (mode == TERM_MODE_INFO) {
        term_info.foreground_color = color;
    }
    return;
}

/**
 * @brief Sets the terminal cursor pos object
 *
 * @param t Terminal to change
 * @param x X-coordinate
 * @param y Y-coordinate
 */
void set_terminal_cursor_pos(TERMINAL *t, uint32_t x, uint32_t y) {
  if (!t) {
    kprintf("Null terminal!\n");
    return;
  }
  if (x >= t->width) {
    y++;
    x = 0;
  }
  t->cursor_pos.x = x;
  t->cursor_pos.y = y;
}

/**
 * @brief Helper to move the terminal cursor forward
 *
 * @param t Terminal to change
 */
void terminal_push_cursor(TERMINAL *t) {
    if (!t) {
        kprintf("Null terminal!\n");
        return;
    }

    if (t->cursor_pos.x >= (int)t->width) {
        t->cursor_pos.x = 0;
        t->cursor_pos.y++;
        return;
    }
    if (t->cursor_pos.y >= (int)t->height &&
        !(t->cursor_pos.y == (int)t->height &&
        t->cursor_pos.x == 0)) {
        terminal_scroll(t);
        t->cursor_pos.y--;
        return;
    }
    t->cursor_pos.x++;
}

/**
 * @brief Helper to move the cursor backwards.
 *
 * @param t Terminal to change
 */
void terminal_pop_cursor(TERMINAL *t) {
    if (!t) {
        kprintf("Null terminal!\n");
        return;
    }

    if (--t->cursor_pos.x < 1) {
        t->cursor_pos.x++;
    }
}

/**
 * @brief Parses ANSI escape sequences
 * @ref https://en.wikipedia.org/wiki/ANSI_escape_code
 * @ref https://wiki.osdev.org/Terminals
 *
 * @param curr Current terminal
 * @param byte Byte as part of the ANSI escape sequences
 * @return STATUS SYS_OK if success, SYS_ERR if fail
 */
STATUS terminal_parse_cmd(TERMINAL *curr, uint8_t byte) {
    if (!curr) {
        kloge("TERMINAL PARSE CMD: Null terminal!\n");
        return SYS_ERR;
    }

    if (curr->state == TERM_STATE_UNKNOWN) {
        return SYS_ERR;
    }

    if (byte > 0xA0 && curr->last_char == 0) {
        curr->last_char = byte;
        goto success;
    }

    if (curr->state == TERM_STATE_IDLE) {
        /* Check if started by part of a Control Sequence Introducer (CSI) */
        if (byte == CSI_START) {
            if (!(curr->last_qu_char)) {
                curr->state = TERM_STATE_CMD;
                curr->last_qu_char = TRUE;
            } else {
                curr->last_qu_char = FALSE;
                goto fail;
            }
        } else {
            goto fail;
        }
    } else if (curr->state == TERM_STATE_CMD) {
        /* Now, we are parsing a ANSI escape sequence when we get here */
        switch (byte) {
            case '[':
                /* This completes the Control Sequence Introducer, set to */
                /* parameter state to grab the arguments of the CSI       */
                curr->cparamcount = 1;
                curr->cparams[0] = 0;
                curr->state = TERM_STATE_PARAM;
                break;
            case ']':
                /* This ends the entire ANSI sequence */
                curr->cparamcount = 0;
                curr->cparams[0] = 0;
                curr->state = TERM_STATE_HYPERLINK_HEADER;
                curr->last_qu_char = FALSE;
                break;
            default:
                goto fail;
        }
    } else if (curr->state == TERM_STATE_PARAM) {
        int fh;
        int fw;
        switch (byte) {
            case ';':
                /* Delimiter between different numbers in a sequence */
                curr->cparams[curr->cparamcount++] = 0;
                break;
            case 'H':
                /* CUP (Cursor Position) Escape Sequences */
                /* Forumla: \033[n;mH where n is a row and m is a column */
                /* If n and m are not present, move the cursor to top left */
                /* of the screen */
                curr->cursor_pos = IVEC2_ZERO;
                goto success;
            case 'K':
                /* EL (Erase in Line) Escape Sequences */
                /* Formula: \033[nK where n can be a number*/
                /* NOTE: **Cursor position doesn't change** */
                /* \033[K or \033[0K -> Clear from cursor to the end of the line */
                /* \033[1K -> Clear from cursor to beginning of line */
                /* \033[2K -> Clear entire line */
                int mode = -1;
                int params = curr->cparamcount;
                int fparam = curr->cparams[0];
                if (params == 0 || (params == 1 && fparam == 0)) {
                    mode = 0;
                } else if (params == 1 && fparam == 1) {
                    mode = 1;
                } else if (params == 1 && fparam == 2) {
                    mode = 2;
                }


                fh = font.header->character_size;
                fw = PSF1_FONT_WIDTH;
                int cond = MIN((curr->cursor_pos.y + 1) * fh, curr->framebuffer.height);
                if (mode > -1) {
                    for (int y = curr->cursor_pos.y * fh; y < cond; y++) {
                        for (int x = 0; x < (int) curr->framebuffer.width; x++) {
                            if (!(mode == 0 && x >= curr->cursor_pos.x * fw)) {
                                continue;
                            } else if (!(mode == 1 && x < curr->cursor_pos.x * fw)) {
                                continue;
                            }
                            fb_putpixel(&(curr->framebuffer), x, y,
                                        curr->background_color);
                        }
                    }
                }
                goto success;
            case 'J':
                /* ED (Erase Display) Escape Sequences */
                /* Formula: \033[nJ where n can be a number */
                /* \033[J or \033[0J -> Clear the screen from cursor to the end */
                /* \033[1J -> Clear the screen from beginning to the cursor */
                /* \033[2J -> Clear the entire screen */
                if (curr->cparamcount != 1) {
                    goto fail;
                }
                fh = font.header->character_size;
                fw = PSF1_FONT_WIDTH;
                switch (curr->cparams[0]) {
                    case 0:
                        /* Clear the screen from cursor to the end */
                        for (int y = 0; y < (int)(curr->framebuffer.height); y++) {
                            for (int x = 0; x < (int)(curr->framebuffer.width); x++) {
                                if (y >= (curr->cursor_pos.y * fh) &&
                                    y < ((curr->cursor_pos.y + 1) * fh) &&
                                    x < (curr->cursor_pos.x * fw)) {
                                        continue;
                                } else if (y < curr->cursor_pos.y * fh) {
                                    continue;
                                }
                                fb_putpixel(&(curr->framebuffer), x, y,
                                            curr->background_color);
                            }
                        }
                        break;
                    case 1:
                        /* Clear the screen from beginning to cursor */
                        for (int y = 0; y < (int)(curr->framebuffer.height); y++) {
                            for (int x = 0; x < (int)(curr->framebuffer.width); x++) {
                                if (y >= (curr->cursor_pos.y * fh) &&
                                    y < ((curr->cursor_pos.y + 1) * fh) &&
                                    x < (curr->cursor_pos.x * fw)) {
                                        continue;
                                } else if (y >= (curr->cursor_pos.y + 1) * fh) {
                                    continue;
                                }
                                fb_putpixel(&(curr->framebuffer), x, y,
                                            curr->background_color);
                            }
                        }
                        break;
                    case 2:
                        /* Clear the entire screen */
                        for (size_t y = 0; y < curr->framebuffer.height; y++) {
                            for (size_t x = 0; x < curr->framebuffer.width; x++) {
                                fb_putpixel(&(curr->framebuffer), x, y,
                                            curr->background_color);
                            }
                        }
                        break;
                    default:
                        goto fail;
                }
                goto success;
            case 'm':
                /* Terminal colors */
                if (curr->cparamcount > 1) {
                    curr->is_bold = curr->cparams[0];
                } else {
                    curr->is_bold = FALSE;
                }

                size_t i = 0;
                if (curr->cparamcount > 0) {
                    i = curr->cparamcount - 1;
                }
                if (curr->cparams[curr->cparamcount - 1] == 0) {
                    /* \033[0m -> Reset all attributes */
                    curr->foreground_color = DEFAULT_FG;
                    curr->background_color = DEFAULT_BG;
                } else if (curr->cparams[i] >= 30 && curr->cparams[i] <= 37) {
                    /* \033[(30 - 37)m -> Set foreground color (3-bit range) */
                    curr->foreground_color = four_bit_colors[curr->cparams[i] - 30];
                } else if (curr->cparams[i] >= 40 && curr->cparams[i] <= 47) {
                    /* \033[(40 - 47)m -> Set background color (3-bit range) */
                    curr->foreground_color = four_bit_colors[curr->cparams[i] - 40];
                } else if (curr->cparams[i] >= 90 && curr->cparams[i] <= 97) {
                    /* \033[(90 - 97)m -> Set foreground color (4-bit range) */
                    curr->foreground_color = four_bit_colors[(curr->cparams[i] - 90) + 7];
                } else if (curr->cparams[i] >= 100 && curr->cparams[i] <= 107) {
                    /* \033[(100 - 107)m -> Set background color (4-bit range) */
                    curr->foreground_color = four_bit_colors[(curr->cparams[i] - 100) + 7];
                }
                goto success;
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                /* Capture the numbers from the parameters */
                curr->cparams[curr->cparamcount - 1] *= 10;
                curr->cparams[curr->cparamcount - 1] += byte - '0';
                break;
            default:
                goto fail;
        }
    } else if (curr->state == TERM_STATE_HYPERLINK_HEADER) {
        /* Here, we're setting up the hyperlink (Operating System Command Sequences) */
        if (byte == ';' && curr->cparamcount == 2) {
            if (curr->cparams[0] == '8' && curr->cparams[1] == ';') {
                curr->state = TERM_STATE_HYPERLINK_URL;
            } else if (curr->cparams[1] == ';') {
                goto fail;
            } else {
                curr->cparams[curr->cparamcount++] = byte;
            }
        } else if (curr->cparamcount > 2) {
            goto fail;
        } else {
            curr->cparams[curr->cparamcount++] = byte;
        }
    } else if (curr->state == TERM_STATE_HYPERLINK_URL) {
        if (byte == OSC_START) {
            curr->state = TERM_STATE_HYPERLINK_TEXT;
        } else {
            /* Skip URL display */
            return SYS_OK;
        }
    } else if (curr->state == TERM_STATE_HYPERLINK_TEXT) {
        if (byte == CSI_START) {
            curr->cparams[0] = 0;
            curr->cparamcount = 0;
            curr->state = TERM_STATE_HYPERLINK_TAIL;
        } else {
            /* Display URL title */
            return SYS_ERR;
        }
    } else if (curr->state == TERM_STATE_HYPERLINK_TAIL) {
        if (byte == OSC_START && curr->cparamcount == 4) {
            if (curr->cparams[0] == ']' &&
                curr->cparams[1] == '8' &&
                curr->cparams[2] == ';' &&
                curr->cparams[3] == ';') {
                goto success;
            } else {
                goto fail;
            }
        } else if (curr->cparamcount > 4) {
            goto fail;
        } else {
            curr->cparams[curr->cparamcount++] = byte;
        }
    } else {
        goto fail;
    }
    return SYS_OK;

    success:
        curr->state = TERM_STATE_IDLE;
        curr->cparamcount = 0;
        return SYS_OK;
    fail:
        curr->state = TERM_STATE_IDLE;
        curr->cparamcount = 0;
        return SYS_ERR;
}

/**
 * @brief Helper to put a character on the screen in the terminal.
 *
 * @param mode Mode is mapped to a terminal
 * @param c Character to put on the screen
 */
void terminal_putc(TERM_MODE mode, uint8_t c) {
    TERMINAL *curr;
    if (mode == TERM_MODE_INFO) {
        curr = &term_info;
    } else if (mode == TERM_MODE_TERM) {
        curr = &term_cli;
    } else {
        kloge("TERMINAL PUTC: Trying to print to a terminal which is unknown!\n");
        return;
    }

    if (curr->state == TERM_STATE_UNKNOWN) {
        return;
    }

    if ((c & 0b10000000) && curr->skip_left == 0 && curr->state == TERM_STATE_IDLE) {
        if (c & 0b11110000) {
            curr->skip_left = 2;
        } else if (c & 0b11100000) {
            curr->skip_left = 1;
        } else if (c & 0b11000000) {
            curr->skip_left = 0;
        }
        return;
    } else if (curr->skip_left != 0) {
        curr->skip_left--;
        return;
    } else if (curr->last_qu_char && c != '[' && c != ']') {
        curr->state = TERM_STATE_IDLE;

        /* Resend the '\033' character */
        terminal_print(mode, CSI_START);
        curr->last_qu_char = FALSE;
        terminal_putc(mode, c);
        return;
    } else {
        curr->last_qu_char = FALSE;
        if (terminal_parse_cmd(curr, c)) {
            return;
        }
    }
    terminal_print(mode, c);
}

/**
 * @brief Helper to put a string on the screen
 *
 * @param t Terminal to change
 * @param s String to put on the screen
 */
void terminal_puts(TERM_MODE mode, const char *s) {
  for (size_t i = 0; s[i]; i++) {
    terminal_putc(mode, s[i]);
  }
}

/**
 * @brief Helper to scroll the terminal down
 *
 * @param t Terminal to scroll
 */
void terminal_scroll(TERMINAL *t) {
    if (!t) {
        kloge("TERM SCROLL: Trying to use a NULL terminal!\n");
        return;
    }

    if (t->state == TERM_STATE_UNKNOWN) {
        return;
    }

    const uint8_t char_size = font.header->character_size;

    for (size_t y = 0; y < (size_t)(t->cursor_pos.y - 1) * char_size; y++) {
        for (size_t x = 0; x < t->framebuffer.width; x++) {
            /* Get the character from *below* and put it in the current position */
            fb_putpixel(&(t->framebuffer), x, y,
                        fb_getpixel(&(t->framebuffer), x, y + char_size));
        }
    }


    for (size_t y = (t->cursor_pos.y - 1) * char_size;
         y < t->framebuffer.height;
         y++) {
        for (size_t x = 0; x < t->framebuffer.width; x++) {
            fb_putpixel(&(t->framebuffer), x, y, t->background_color);
        }
    }
}

/**
 * @brief Sets the cursor to a specific character
 *
 * @param c Character to set the cursor to
 */
void terminal_set_cursor(uint8_t c) {
    cursor_visible = c;
}


/**
 * @brief Returns the current mode that the terminal is set to
 *
 * @return TERM_MODE Terminal mode the terminal is set to
 */
TERM_MODE terminal_get_mode() {
    return term_mode;
}


/* --------------------------- WINSIZE FUNCTIONALITY -------------------------*/

/**
 * @brief Gets WINDOW_SIZE
 *
 * @param ws Buffer to copy WINDOW_SIZE into
 */
void terminal_get_winsize(WINDOW_SIZE *ws) {
    if (!ws) {
        return;
    }

    if (term_cli.state == TERM_STATE_IDLE) {
        ws->col = term_cli.width;
        ws->row = term_cli.height;
        ws->xpixel = term_cli.framebuffer.width;
        ws->ypixel = term_cli.framebuffer.height;
    }
}


/**
 * @brief Sets the WINDOW_SIZE
 *
 * @param ws Window size to set
 * @return STATUS SYS_ERR if failure, SYS_OK if success
 */
STATUS terminal_set_winsize(WINDOW_SIZE *ws) {
    if (!ws) {
        return SYS_ERR;
    }

    if (term_cli.state == TERM_STATE_IDLE) {
        if (ws->col != term_cli.width ||
            ws->row != term_cli.height ||
            ws->xpixel != term_cli.framebuffer.width ||
            ws->ypixel != term_cli.framebuffer.height) {
            kloge("TERMINAL SET WINSIZE: Can't support specified terminal window size\n");
        } else {
            term_cli.width = ws->col;
            term_cli.height = ws->row;
            term_cli.framebuffer.width = ws->xpixel;
            term_cli.framebuffer.height = ws->ypixel;
            return SYS_OK;
        }
    }

    return SYS_ERR;
}