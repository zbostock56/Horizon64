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

/* --------------------------- CORE FUNCTIONS ------------------------------- */

/**
 * @brief Initialize terminal subsystem
 *
 * @param fb Framebuffer to use for terminal display
 */
void init_terminal(struct limine_framebuffer *fb);

/**
 * @brief Start terminal operation
 */
void terminal_start(void);

/**
 * @brief Clear terminal screen
 *
 * @param mode Terminal mode to clear
 */
void terminal_clear(TERM_MODE mode);

/**
 * @brief Refresh terminal display
 *
 * @param mode Terminal mode to refresh
 * @return STATUS SYS_OK on success, SYS_ERR on failure
 */
STATUS terminal_refresh(TERM_MODE mode);

/* --------------------------- CHARACTER I/O -------------------------------- */

/**
 * @brief Print a character to terminal (low-level)
 *
 * @param mode Terminal mode
 * @param c Character to print
 */
void terminal_print(TERM_MODE mode, uint8_t c);

/**
 * @brief Put a character to terminal (with escape sequence processing)
 *
 * @param mode Terminal mode
 * @param c Character to put
 */
void terminal_putc(TERM_MODE mode, uint8_t c);

/**
 * @brief Put a string to terminal
 *
 * @param mode Terminal mode
 * @param s String to put
 */
void terminal_puts(TERM_MODE mode, const char *s);

/**
 * @brief Print formatted string to terminal
 *
 * @param mode Terminal mode
 * @param format Format string
 * @param ... Variable arguments
 */
void terminal_printf(TERM_MODE mode, const char *format, ...);

/**
 * @brief Print formatted string to terminal with va_list
 *
 * @param mode Terminal mode
 * @param format Format string
 * @param args Variable argument list
 */
void terminal_vprintf(TERM_MODE mode, const char *format, va_list args);

/* --------------------------- CURSOR CONTROL ------------------------------- */

/**
 * @brief Set terminal cursor position
 *
 * @param t Terminal instance
 * @param x X coordinate (column)
 * @param y Y coordinate (row)
 */
void set_terminal_cursor_pos(TERMINAL *t, uint32_t x, uint32_t y);

/**
 * @brief Move cursor forward (right/down with wrap)
 *
 * @param t Terminal instance
 */
void terminal_push_cursor(TERMINAL *t);

/**
 * @brief Move cursor backward (left/up with wrap)
 *
 * @param t Terminal instance
 */
void terminal_pop_cursor(TERMINAL *t);

/**
 * @brief Set cursor character/shape
 *
 * @param c Cursor character (0 for invisible)
 */
void terminal_set_cursor(uint8_t c);

/**
 * @brief Set cursor visibility
 *
 * @param mode Terminal mode
 * @param visible Cursor visibility state
 */
void terminal_set_cursor_visibility(TERM_MODE mode, TERM_CURSOR_STATUS visible);

/**
 * @brief Get cursor position
 *
 * @param mode Terminal mode
 * @param x Pointer to store X coordinate
 * @param y Pointer to store Y coordinate
 * @return STATUS SYS_OK on success, SYS_ERR on failure
 */
STATUS terminal_get_cursor_pos(TERM_MODE mode, uint32_t *x, uint32_t *y);

/* --------------------------- DISPLAY CONTROL ------------------------------ */

/**
 * @brief Scroll terminal content
 *
 * @param t Terminal instance
 */
void terminal_scroll(TERMINAL *t);

/**
 * @brief Scroll terminal up by specified number of lines
 *
 * @param mode Terminal mode
 * @param lines Number of lines to scroll
 */
void terminal_scroll_up(TERM_MODE mode, uint32_t lines);

/**
 * @brief Scroll terminal down by specified number of lines
 *
 * @param mode Terminal mode
 * @param lines Number of lines to scroll
 */
void terminal_scroll_down(TERM_MODE mode, uint32_t lines);

/**
 * @brief Set scroll region
 *
 * @param mode Terminal mode
 * @param top Top line of scroll region
 * @param bottom Bottom line of scroll region
 * @return STATUS SYS_OK on success, SYS_ERR on failure
 */
STATUS terminal_set_scroll_region(TERM_MODE mode, uint32_t top, uint32_t bottom);

/**
 * @brief Reset scroll region to full screen
 *
 * @param mode Terminal mode
 */
void terminal_reset_scroll_region(TERM_MODE mode);

/* --------------------------- COLOR AND ATTRIBUTES ------------------------- */

/**
 * @brief Set terminal foreground color
 *
 * @param mode Terminal mode
 * @param color Foreground color
 */
void terminal_set_foreground(TERM_MODE mode, FRAMEBUFFER_COLORS color);

/**
 * @brief Set terminal background color
 *
 * @param mode Terminal mode
 * @param color Background color
 */
void terminal_set_background(TERM_MODE mode, FRAMEBUFFER_COLORS color);

/**
 * @brief Set terminal colors (foreground and background)
 *
 * @param mode Terminal mode
 * @param fg Foreground color
 * @param bg Background color
 */
void terminal_set_colors(TERM_MODE mode, FRAMEBUFFER_COLORS fg, FRAMEBUFFER_COLORS bg);

/**
 * @brief Reset terminal colors to default
 *
 * @param mode Terminal mode
 */
void terminal_reset_colors(TERM_MODE mode);

/**
 * @brief Set text attributes
 *
 * @param mode Terminal mode
 * @param bold Enable/disable bold
 * @param underline Enable/disable underline
 * @param reverse Enable/disable reverse video
 */
void terminal_set_attributes(TERM_MODE mode, bool bold, bool underline, bool reverse);

/**
 * @brief Reset all text attributes
 *
 * @param mode Terminal mode
 */
void terminal_reset_attributes(TERM_MODE mode);

/* --------------------------- ESCAPE SEQUENCE PROCESSING ------------------- */

/**
 * @brief Parse terminal command/escape sequence
 *
 * @param curr Terminal instance
 * @param byte Input byte
 * @return STATUS SYS_OK on success, SYS_ERR on failure
 */
STATUS terminal_parse_cmd(TERMINAL *curr, uint8_t byte);

/**
 * @brief Set graphics mode (colors, attributes)
 *
 * @param curr Terminal instance
 * @return STATUS SYS_OK on success, SYS_ERR on failure
 */
STATUS terminal_set_graphics_mode(TERMINAL *curr);

/**
 * @brief Reset terminal to default state
 *
 * @param curr Terminal instance
 */
void terminal_reset_state(TERMINAL *curr);

/* --------------------------- TERMINAL MANAGEMENT -------------------------- */

/**
 * @brief Get terminal instance by mode
 *
 * @param mode Terminal mode
 * @return TERMINAL* Terminal instance or NULL if invalid mode
 */
TERMINAL *terminal_get_by_mode(TERM_MODE mode);

/**
 * @brief Get current terminal mode
 *
 * @return TERM_MODE Current active terminal mode
 */
TERM_MODE terminal_get_mode(void);

/**
 * @brief Check if terminal needs redraw
 *
 * @return uint8_t TRUE if redraw needed, FALSE otherwise
 */
uint8_t terminal_need_redraw(void);

/**
 * @brief Set terminal redraw flag
 *
 * @param flag Redraw flag value
 */
void terminal_set_redraw(uint8_t flag);

/**
 * @brief Enable character printing to framebuffer
 */
void terminal_enable_character_printing(void);

/**
 * @brief Disable character printing to framebuffer
 */
void terminal_disable_character_printing(void);

/**
 * @brief Check if character printing is enabled
 *
 * @return bool TRUE if enabled, FALSE otherwise
 */
bool terminal_is_character_printing_enabled(void);

/* --------------------------- WINDOW SIZE FUNCTIONALITY ------------------- */

/**
 * @brief Get terminal window size
 *
 * @param ws Pointer to WINDOW_SIZE structure to fill
 * @return STATUS SYS_OK on success, SYS_ERR on failure
 */
STATUS terminal_get_winsize(WINDOW_SIZE *ws);

/**
 * @brief Set terminal window size
 *
 * @param ws Pointer to WINDOW_SIZE structure with new size
 * @return STATUS SYS_OK on success, SYS_ERR on failure
 */
STATUS terminal_set_winsize(WINDOW_SIZE *ws);

/**
 * @brief Get terminal dimensions in characters
 *
 * @param mode Terminal mode
 * @param width Pointer to store width
 * @param height Pointer to store height
 * @return STATUS SYS_OK on success, SYS_ERR on failure
 */
STATUS terminal_get_dimensions(TERM_MODE mode, uint32_t *width, uint32_t *height);

/**
 * @brief Get terminal dimensions in pixels
 *
 * @param mode Terminal mode
 * @param width Pointer to store pixel width
 * @param height Pointer to store pixel height
 * @return STATUS SYS_OK on success, SYS_ERR on failure
 */
STATUS terminal_get_pixel_dimensions(TERM_MODE mode, uint32_t *width, uint32_t *height);

/* --------------------------- CONFIGURATION -------------------------------- */

/**
 * @brief Set terminal feature flags
 *
 * @param mode Terminal mode
 * @param features Feature flags to set
 */
void terminal_set_features(TERM_MODE mode, uint32_t features);

/**
 * @brief Get terminal feature flags
 *
 * @param mode Terminal mode
 * @return uint32_t Current feature flags
 */
uint32_t terminal_get_features(TERM_MODE mode);

/**
 * @brief Set terminal capability flags
 *
 * @param mode Terminal mode
 * @param capabilities Capability flags to set
 */
void terminal_set_capabilities(TERM_MODE mode, uint32_t capabilities);

/**
 * @brief Get terminal capability flags
 *
 * @param mode Terminal mode
 * @return uint32_t Current capability flags
 */
uint32_t terminal_get_capabilities(TERM_MODE mode);

/**
 * @brief Set terminal character set
 *
 * @param mode Terminal mode
 * @param charset Character set to use
 */
void terminal_set_charset(TERM_MODE mode, TERM_CHARSET charset);

/**
 * @brief Get terminal character set
 *
 * @param mode Terminal mode
 * @return TERM_CHARSET Current character set
 */
TERM_CHARSET terminal_get_charset(TERM_MODE mode);

/**
 * @brief Set tab width
 *
 * @param mode Terminal mode
 * @param width Tab width in characters
 */
void terminal_set_tab_width(TERM_MODE mode, uint32_t width);

/**
 * @brief Get tab width
 *
 * @param mode Terminal mode
 * @return uint32_t Current tab width
 */
uint32_t terminal_get_tab_width(TERM_MODE mode);

/* --------------------------- STATISTICS AND DEBUGGING -------------------- */

/**
 * @brief Get terminal statistics
 *
 * @param mode Terminal mode
 * @param stats Pointer to statistics structure to fill
 * @return STATUS SYS_OK on success, SYS_ERR on failure
 */
STATUS terminal_get_stats(TERM_MODE mode, TERMINAL_STATS *stats);

/**
 * @brief Get global terminal statistics
 *
 * @param stats Pointer to statistics structure to fill
 * @return STATUS SYS_OK on success, SYS_ERR on failure
 */
STATUS terminal_get_global_stats(TERMINAL_STATS *stats);

/**
 * @brief Reset terminal statistics
 *
 * @param mode Terminal mode
 */
void terminal_reset_stats(TERM_MODE mode);

/**
 * @brief Reset global terminal statistics
 */
void terminal_reset_global_stats(void);

/**
 * @brief Get terminal information for debugging
 *
 * @param mode Terminal mode
 * @param buffer Buffer to store information string
 * @param buffer_size Size of buffer
 * @return STATUS SYS_OK on success, SYS_ERR on failure
 */
STATUS terminal_get_info(TERM_MODE mode, char *buffer, size_t buffer_size);

/**
 * @brief Print terminal statistics to specified terminal
 *
 * @param output_mode Terminal to print to
 * @param stats_mode Terminal to get stats from
 */
void terminal_print_stats(TERM_MODE output_mode, TERM_MODE stats_mode);

/**
 * @brief Print global terminal statistics
 *
 * @param output_mode Terminal to print to
 */
void terminal_print_global_stats(TERM_MODE output_mode);

/* --------------------------- ADVANCED FEATURES ---------------------------- */

/**
 * @brief Set terminal title (if supported)
 *
 * @param mode Terminal mode
 * @param title Title string
 */
void terminal_set_title(TERM_MODE mode, const char *title);

/**
 * @brief Ring terminal bell
 *
 * @param mode Terminal mode
 */
void terminal_bell(TERM_MODE mode);

/**
 * @brief Insert blank lines at cursor position
 *
 * @param mode Terminal mode
 * @param count Number of lines to insert
 */
void terminal_insert_lines(TERM_MODE mode, uint32_t count);

/**
 * @brief Delete lines at cursor position
 *
 * @param mode Terminal mode
 * @param count Number of lines to delete
 */
void terminal_delete_lines(TERM_MODE mode, uint32_t count);

/**
 * @brief Insert blank characters at cursor position
 *
 * @param mode Terminal mode
 * @param count Number of characters to insert
 */
void terminal_insert_chars(TERM_MODE mode, uint32_t count);

/**
 * @brief Delete characters at cursor position
 *
 * @param mode Terminal mode
 * @param count Number of characters to delete
 */
void terminal_delete_chars(TERM_MODE mode, uint32_t count);

/**
 * @brief Set terminal to alternate screen buffer (if supported)
 *
 * @param mode Terminal mode
 * @param alternate TRUE for alternate buffer, FALSE for normal
 */
void terminal_set_alternate_buffer(TERM_MODE mode, bool alternate);

/**
 * @brief Enable/disable auto-wrap mode
 *
 * @param mode Terminal mode
 * @param enable TRUE to enable auto-wrap, FALSE to disable
 */
void terminal_set_auto_wrap(TERM_MODE mode, bool enable);

/**
 * @brief Enable/disable insert mode
 *
 * @param mode Terminal mode
 * @param enable TRUE for insert mode, FALSE for replace mode
 */
void terminal_set_insert_mode(TERM_MODE mode, bool enable);

/**
 * @brief Set origin mode (cursor positioning relative to scroll region)
 *
 * @param mode Terminal mode
 * @param enable TRUE for origin mode, FALSE for absolute positioning
 */
void terminal_set_origin_mode(TERM_MODE mode, bool enable);

/* --------------------------- INPUT HANDLING ------------------------------- */

/**
 * @brief Handle keyboard input for terminal
 *
 * @param mode Terminal mode
 * @param key Key code
 * @param modifiers Modifier keys (shift, ctrl, alt)
 * @return STATUS SYS_OK if handled, SYS_ERR otherwise
 */
STATUS terminal_handle_key(TERM_MODE mode, uint32_t key, uint32_t modifiers);

/**
 * @brief Handle mouse input for terminal
 *
 * @param mode Terminal mode
 * @param x Mouse X coordinate
 * @param y Mouse Y coordinate
 * @param buttons Mouse button state
 * @return STATUS SYS_OK if handled, SYS_ERR otherwise
 */
STATUS terminal_handle_mouse(TERM_MODE mode, uint32_t x, uint32_t y, uint32_t buttons);

/* --------------------------- UTILITY FUNCTIONS ---------------------------- */

/**
 * @brief Convert ANSI color code to framebuffer color
 *
 * @param ansi_color ANSI color code (0-15 for 4-bit, 0-255 for 8-bit)
 * @param is_background TRUE if this is a background color
 * @return FRAMEBUFFER_COLORS Corresponding framebuffer color
 */
FRAMEBUFFER_COLORS terminal_ansi_to_fb_color(uint8_t ansi_color, bool is_background);

/**
 * @brief Convert RGB values to framebuffer color
 *
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @return FRAMEBUFFER_COLORS RGB color as framebuffer color
 */
FRAMEBUFFER_COLORS terminal_rgb_to_fb_color(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Check if character is printable
 *
 * @param c Character to check
 * @return bool TRUE if printable, FALSE otherwise
 */
bool terminal_is_printable(uint8_t c);

/**
 * @brief Check if character is control character
 *
 * @param c Character to check
 * @return bool TRUE if control character, FALSE otherwise
 */
bool terminal_is_control(uint8_t c);

/**
 * @brief Get character width (for Unicode support)
 *
 * @param c Character to check
 * @return uint32_t Character width in terminal cells
 */
uint32_t terminal_char_width(uint32_t c);

/* --------------------------- LEGACY COMPATIBILITY ------------------------- */

/**
 * @brief Legacy function aliases for backward compatibility
 */
#define terminal_putchar(mode, c)           terminal_putc(mode, c)
#define terminal_print_string(mode, s)      terminal_puts(mode, s)
#define terminal_get_current_mode()         terminal_get_mode()
#define terminal_set_current_mode(mode)     terminal_set_mode(mode)

/* --------------------------- INLINE HELPERS ------------------------------- */

