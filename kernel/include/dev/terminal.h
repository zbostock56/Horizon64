/**
 * @file terminal.h
 * @author Zack Bostock
 * @brief Information pertaining to terminal functionality
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#include <globals.h>
#include <stdarg.h>
#include <stdint.h>
#include <structs/terminal_str.h>
#include <structs/termios_str.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */

/**
 * @brief Global cursor visibility state
 */
extern TERM_CURSOR_STATUS cursor_visible;

/* --------------------------------- MACROS --------------------------------- */

/**
 * @brief Check if terminal mode is valid
 */
#define TERMINAL_MODE_VALID(mode) \
    ((mode) == TERM_MODE_INFO || (mode) == TERM_MODE_TERM)

/**
 * @brief Check if coordinates are within terminal bounds
 */
#define TERMINAL_COORDS_VALID(term, x, y) \
    ((term) && (x) >= 0 && (y) >= 0 && (x) < (int)(term)->width && (y) < (int)(term)->height)

/**
 * @brief Get terminal character cell count
 */
#define TERMINAL_CELL_COUNT(term) \
    ((term) ? (term)->width * (term)->height : 0)

/**
 * @brief Convert character coordinates to pixel coordinates
 */
#define TERMINAL_CHAR_TO_PIXEL_X(x) ((x) * PSF1_FONT_WIDTH)
#define TERMINAL_CHAR_TO_PIXEL_Y(y) ((y) * font.header->character_size)

/**
 * @brief Convert pixel coordinates to character coordinates
 */
#define TERMINAL_PIXEL_TO_CHAR_X(x) ((x) / PSF1_FONT_WIDTH)
#define TERMINAL_PIXEL_TO_CHAR_Y(y) ((y) / font.header->character_size)

/* --------------------------- INTERNALLY DEFINED --------------------------- */
/* Core functions */
void init_terminal(struct limine_framebuffer *fb);
void terminal_start(void);
void terminal_clear(TERM_MODE mode);
STATUS terminal_refresh(TERM_MODE mode);

/* Character I/O */
void terminal_print(TERM_MODE mode, uint8_t c);
void terminal_putc(TERM_MODE mode, uint8_t c);
void terminal_puts(TERM_MODE mode, const char *s);
void terminal_printf(TERM_MODE mode, const char *format, ...);
void terminal_vprintf(TERM_MODE mode, const char *format, va_list args);

/* Cursor control */
void set_terminal_cursor_pos(TERMINAL *t, uint32_t x, uint32_t y);
void terminal_push_cursor(TERMINAL *t);
void terminal_pop_cursor(TERMINAL *t);
void terminal_set_cursor(uint8_t c);
void terminal_set_cursor_visibility(TERM_MODE mode, TERM_CURSOR_STATUS visible);
STATUS terminal_get_cursor_pos(TERM_MODE mode, uint32_t *x, uint32_t *y);

/* Display control */
void terminal_scroll(TERMINAL *t);
void terminal_scroll_up(TERM_MODE mode, uint32_t lines);
void terminal_scroll_down(TERM_MODE mode, uint32_t lines);
STATUS terminal_set_scroll_region(TERM_MODE mode, uint32_t top, uint32_t bottom);
void terminal_reset_scroll_region(TERM_MODE mode);

/* Color and attributes */
void terminal_set_foreground(TERM_MODE mode, FRAMEBUFFER_COLORS color);
void terminal_set_background(TERM_MODE mode, FRAMEBUFFER_COLORS color);
void terminal_set_colors(TERM_MODE mode, FRAMEBUFFER_COLORS fg, FRAMEBUFFER_COLORS bg);
void terminal_reset_colors(TERM_MODE mode);
void terminal_set_attributes(TERM_MODE mode, bool bold, bool underline, bool reverse);
void terminal_reset_attributes(TERM_MODE mode);

/* Escape sequence processing */
STATUS terminal_parse_cmd(TERMINAL *curr, uint8_t byte);
STATUS terminal_set_graphics_mode(TERMINAL *curr);

/* Terminal management */
uint8_t terminal_need_redraw(void);
void terminal_set_redraw(uint8_t flag);
void terminal_enable_character_printing(void);
void terminal_disable_character_printing(void);
bool terminal_is_character_printing_enabled(void);

/* Window size (WINSZ) */
STATUS terminal_get_winsize(WINDOW_SIZE *ws);
STATUS terminal_set_winsize(WINDOW_SIZE *ws);
STATUS terminal_get_dimensions(TERM_MODE mode, uint32_t *width, uint32_t *height);
STATUS terminal_get_pixel_dimensions(TERM_MODE mode, uint32_t *width, uint32_t *height);

/* Configuration */
void terminal_set_features(TERM_MODE mode, uint32_t features);
uint32_t terminal_get_features(TERM_MODE mode);
void terminal_set_capabilities(TERM_MODE mode, uint32_t capabilities);
uint32_t terminal_get_capabilities(TERM_MODE mode);
void terminal_set_charset(TERM_MODE mode, TERM_CHARSET charset);
TERM_CHARSET terminal_get_charset(TERM_MODE mode);
void terminal_set_tab_width(TERM_MODE mode, uint32_t width);
uint32_t terminal_get_tab_width(TERM_MODE mode);

/* Stats and debugging */
STATUS terminal_get_stats(TERM_MODE mode, TERMINAL_STATS *stats);
STATUS terminal_get_global_stats(TERMINAL_STATS *stats);
void terminal_reset_stats(TERM_MODE mode);
void terminal_reset_global_stats(void);
STATUS terminal_get_info(TERM_MODE mode, char *buffer, size_t buffer_size);
void terminal_print_stats(TERM_MODE output_mode, TERM_MODE stats_mode);
void terminal_print_global_stats(TERM_MODE output_mode);

/* Advanced features */
void terminal_set_title(TERM_MODE mode, const char *title);
void terminal_bell(TERM_MODE mode);
void terminal_insert_lines(TERM_MODE mode, uint32_t count);
void terminal_delete_lines(TERM_MODE mode, uint32_t count);
void terminal_delete_chars(TERM_MODE mode, uint32_t count);
void terminal_set_alternate_buffer(TERM_MODE mode, bool alternate);
void terminal_set_auto_wrap(TERM_MODE mode, bool enable);
void terminal_set_insert_mode(TERM_MODE mode, bool enable);
void terminal_set_origin_mode(TERM_MODE mode, bool enable);

/* Input handling */
STATUS terminal_handle_key(TERM_MODE mode, uint32_t key, uint32_t modifiers);
STATUS terminal_handle_mouse(TERM_MODE mode, uint32_t x, uint32_t y, uint32_t buttons);

/* Utility functions */
FRAMEBUFFER_COLORS terminal_ansi_to_fb_color(uint8_t ansi_color, bool is_background);
FRAMEBUFFER_COLORS terminal_rgb_to_fb_color(uint8_t r, uint8_t g, uint8_t b);
bool terminal_is_printable(uint8_t c);
bool terminal_is_control(uint8_t c);
uint32_t terminal_char_width(uint32_t c);

/* Interaction with ttyfs driver */
int64_t terminal_ioctl(int64_t request, int64_t arg);

/* --------------------------- LEGACY COMPATIBILITY ------------------------- */

/**
 * @brief Legacy function aliases for backward compatibility
 */
#define terminal_putchar(mode, c)           terminal_putc(mode, c)
#define terminal_print_string(mode, s)      terminal_puts(mode, s)
#define terminal_get_current_mode()         terminal_get_mode()

/* --------------------------- INLINE HELPERS ------------------------------- */

