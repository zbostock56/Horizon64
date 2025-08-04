/**
 * @file framebuffer.c
 * @author Zack Bostock
 * @brief Enhanced framebuffer implementation with performance optimizations
 * @verbatim
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <common/kmalloc.h>
#include <common/string.h>
#include <common/memory.h>
#include <common/kprint.h>

#include <sys/asm.h>

#include <graphics/framebuffer.h>

/**
 * @brief Fast memory copy optimized for framebuffer operations
 * Uses 64-bit copies when possible for better performance
 *
 * @param dest Destination buffer
 * @param src Source buffer
 * @param size Number of bytes to copy
 */
static inline void fb_fast_memcpy(void *dest, const void *src, size_t size) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;

    /* Copy 64 bits at a time when aligned */
    if (((uintptr_t)d & 7) == 0 && ((uintptr_t)s & 7) == 0) {
        size_t qwords = size / 8;
        uint64_t *d64 = (uint64_t *)d;
        const uint64_t *s64 = (const uint64_t *)s;

        for (size_t i = 0; i < qwords; i++) {
            d64[i] = s64[i];
        }

        d += qwords * 8;
        s += qwords * 8;
        size -= qwords * 8;
    }

    /* Copy remaining bytes */
    for (size_t i = 0; i < size; i++) {
        d[i] = s[i];
    }
}

/**
 * @brief Fast memory set optimized for framebuffer operations
 * Uses 64-bit writes when possible
 *
 * @param dest Destination buffer
 * @param value 32-bit color value to set
 * @param pixels Number of pixels to set
 */
static inline void fb_fast_memset32(uint32_t *dest, uint32_t value, size_t pixels) {
    /* Use 64-bit writes when possible */
    if (((uintptr_t)dest & 7) == 0) {
        uint64_t value64 = ((uint64_t)value << 32) | value;
        uint64_t *dest64 = (uint64_t *)dest;
        size_t qwords = pixels / 2;

        for (size_t i = 0; i < qwords; i++) {
            dest64[i] = value64;
        }

        dest += qwords * 2;
        pixels -= qwords * 2;
    }

    /* Handle remaining pixels */
    for (size_t i = 0; i < pixels; i++) {
        dest[i] = value;
    }
}

/**
 * @brief Bounds checking for pixel coordinates
 *
 * @param fb Framebuffer to check bounds for
 * @param x X-coordinate
 * @param y Y-coordinate
 * @return true if coordinates are valid, false otherwise
 */
static inline bool fb_bounds_check(const FRAMEBUFFER *fb, uint32_t x, uint32_t y) {
    return (x < fb->width && y < fb->height);
}

/**
 * @brief Check if framebuffer is in valid state for drawing operations
 *
 * @param fb Framebuffer to validate
 * @return true if valid, false otherwise
 */
static inline bool fb_is_valid(const FRAMEBUFFER *fb) {
    return (fb && fb->backbuffer && fb->base &&
            (uint64_t)fb->base != (uint64_t)fb->backbuffer);
}

/**
 * @brief Initialization of a framebuffer
 *
 * @param fb Framebuffer to initialize
 * @param f Limine framebuffer struct
 * @return SYS_OK on success, SYS_ERR on error
 */
STATUS fb_init(FRAMEBUFFER *fb, struct limine_framebuffer *f) {
    if (!fb) {
        kloge("INIT FRAMEBUFFER: Invalid framebuffer pointer\n");
        return SYS_ERR;
    }

    klogs("INIT FRAMEBUFFER: starting...\n");

    if (!f) {
        /* Handle case where we're reinitializing */
        if (fb->base && fb->backbuffer &&
            (uint64_t)fb->base == (uint64_t)fb->backbuffer) {

            uint8_t *new_backbuffer = kmalloc(fb->backbuffer_length);
            if (!new_backbuffer) {
                klogs("INIT FRAMEBUFFER: Failed to allocate backbuffer\n");
                return SYS_ERR;
            }

            fb_fast_memcpy(new_backbuffer, fb->base, fb->backbuffer_length);
            fb->backbuffer = new_backbuffer;
        }
        return SYS_OK;
    }

    /* Validate Limine framebuffer */
    if (!f->address || f->width == 0 || f->height == 0 || f->pitch == 0) {
        klogs("INIT FRAMEBUFFER: Invalid Limine framebuffer parameters\n");
        return SYS_ERR;
    }

    /* Copy over the information from the Limine structure */
    fb->base = f->address;
    fb->swapbuffer = NULL;
    fb->width = f->width;
    fb->height = f->height;
    fb->pitch = f->pitch;

    /* Calculate backbuffer size and allocate */
    fb->backbuffer_length = fb->height * fb->pitch;
    fb->backbuffer = kmalloc(fb->backbuffer_length);

    if (!fb->backbuffer) {
        klogs("INIT FRAMEBUFFER: Failed to allocate backbuffer\n");
        return SYS_ERR;
    }

    fb->background_buffer = NULL;

    /* Clear backbuffer to default background color - optimized version */
    uint32_t total_pixels = (fb->backbuffer_length / sizeof(uint32_t));
    fb_fast_memset32((uint32_t *)fb->backbuffer, DEFAULT_BG, total_pixels);

    /* Copy backbuffer to front buffer */
    fb_refresh(fb);

    klogs("INIT FRAMEBUFFER: finished successfully\n");
    return SYS_OK;
}

/**
 * @brief Optimized putpixel implementation with bounds checking
 *
 * @param fb Framebuffer to modify
 * @param x X-coordinate on the screen
 * @param y Y-coordinate on the screen
 * @param color Color to set
 */
void fb_putpixel(FRAMEBUFFER *fb, uint32_t x, uint32_t y, uint32_t color) {
    if (!fb_is_valid(fb) || !fb_bounds_check(fb, x, y)) {
        return;
    }

    ((uint32_t *)(fb->backbuffer + (fb->pitch * y)))[x] = color;
}

/**
 * @brief Optimized pixel reading with bounds checking
 *
 * @param fb Framebuffer to read from
 * @param x X-coordinate on the screen
 * @param y Y-coordinate on the screen
 * @return uint32_t Color value of the pixel, or 0 if invalid
 */
uint32_t fb_getpixel(FRAMEBUFFER *fb, uint32_t x, uint32_t y) {
    if (!fb_is_valid(fb) || !fb_bounds_check(fb, x, y)) {
        return 0;
    }

    return ((uint32_t *)(fb->backbuffer + (fb->pitch * y)))[x];
}

/**
 * @brief Fast rectangle fill operation
 *
 * @param fb Framebuffer to draw to
 * @param x Starting X coordinate
 * @param y Starting Y coordinate
 * @param width Width of rectangle
 * @param height Height of rectangle
 * @param color Fill color
 */
void fb_fill_rect(FRAMEBUFFER *fb, uint32_t x, uint32_t y,
                  uint32_t width, uint32_t height, uint32_t color) {
    if (!fb_is_valid(fb)) {
        return;
    }

    /* Clip rectangle to framebuffer bounds */
    if (x >= fb->width || y >= fb->height) {
        return;
    }

    if (x + width > fb->width) {
        width = fb->width - x;
    }
    if (y + height > fb->height) {
        height = fb->height - y;
    }

    /* Fill each row of the rectangle */
    for (uint32_t row = 0; row < height; row++) {
        uint32_t *line = (uint32_t *)(fb->backbuffer + ((y + row) * fb->pitch));
        fb_fast_memset32(&line[x], color, width);
    }
}

/**
 * @brief Clear entire framebuffer to specified color
 *
 * @param fb Framebuffer to clear
 * @param color Color to clear to
 */
void fb_clear(FRAMEBUFFER *fb, uint32_t color) {
    if (!fb_is_valid(fb)) {
        return;
    }

    uint32_t total_pixels = (fb->backbuffer_length / sizeof(uint32_t));
    fb_fast_memset32((uint32_t *)fb->backbuffer, color, total_pixels);
}

/**
 * @brief Optimized character drawing with better error handling
 *
 * @param fb Framebuffer to write to
 * @param fg Foreground color
 * @param bg Background color
 * @param str String to write
 */
void fb_draw_characters(FRAMEBUFFER *fb, uint32_t fg, uint32_t bg,
                        const char *str) {
    if (!fb_is_valid(fb) || !str) {
        return;
    }

    size_t str_len = strlen(str);
    if (str_len == 0) {
        return;
    }

    /* Calculate centered position with bounds checking */
    uint32_t char_width = 8 * 6;  /* 8 pixels wide, scaled 6x */
    uint32_t char_height = 16 * 6; /* 16 pixels tall, scaled 6x */
    uint32_t total_width = str_len * char_width;

    if (total_width > fb->width || char_height > fb->height) {
        return; /* String too large for framebuffer */
    }

    uint32_t x = (fb->width - total_width) / 2;
    uint32_t y = (fb->height - char_height) / 2;

    static const uint8_t masks[8] = { 128, 64, 32, 16, 8, 4, 2, 1 };

    for (size_t i = 0; i < str_len; i++) {
        uint32_t char_x = x + i * char_width;
        uint32_t offset = ((uint32_t)str[i]) * font.header->character_size;

        for (size_t j = 0; j < font.header->character_size; j++) {
            uint8_t glyph_row = ((uint8_t *)font.glyph_buffer)[offset + j];
            uint32_t row_y = y + j * 6; /* 6x scaling */

            /* Draw each pixel of the glyph row, scaled 6x6 */
            for (size_t k = 0; k < 8; k++) {
                uint32_t color = (glyph_row & masks[k]) ? fg : bg;
                uint32_t pixel_x = char_x + k * 6;

                /* Draw 6x6 block for each font pixel */
                for (int dy = 0; dy < 6; dy++) {
                    for (int dx = 0; dx < 6; dx++) {
                        if (pixel_x + dx < fb->width && row_y + dy < fb->height) {
                            fb_putpixel(fb, pixel_x + dx, row_y + dy, color);
                        }
                    }
                }
            }
        }
    }
}

/**
 * @brief Optimized single character drawing
 *
 * @param fb Framebuffer to change
 * @param x X-coordinate on the screen
 * @param y Y-coordinate on the screen
 * @param fgcolor Foreground color of the character
 * @param bgcolor Background color of the character
 * @param ch Character to put on the screen
 * @param is_bold Denotes whether character should be bold or not
 */
void fb_putc(FRAMEBUFFER *fb, uint32_t x, uint32_t y, uint32_t fgcolor,
             uint32_t bgcolor, uint8_t ch, uint8_t is_bold) {
    if (!fb_is_valid(fb)) {
        return;
    }

    /* Bounds check for character placement */
    if (x + 8 > fb->width || y + font.header->character_size > fb->height) {
        return;
    }

    uint32_t offset = ((uint32_t)ch) * font.header->character_size;
    static const uint8_t masks[8] = { 128, 64, 32, 16, 8, 4, 2, 1 };

    for (size_t i = 0; i < font.header->character_size; i++) {
        uint8_t glyph_row = ((uint8_t *)font.glyph_buffer)[offset + i];
        uint32_t row_y = y + i;

        for (size_t k = 0; k < 8; k++) {
            uint32_t color = (glyph_row & masks[k]) ? fgcolor : bgcolor;
            uint32_t pixel_x = x + k;

            /* Bold rendering: draw pixel and one to the right */
            fb_putpixel(fb, pixel_x, row_y, color);
            if (is_bold && pixel_x + 1 < fb->width &&
                (glyph_row & masks[k])) {
                fb_putpixel(fb, pixel_x + 1, row_y, fgcolor);
            }
        }
    }
}

/**
 * @brief Optimized framebuffer refresh
 *
 * @param fb Framebuffer struct to refresh
 */
void fb_refresh(FRAMEBUFFER *fb) {
    if (!fb || !fb->base || !fb->backbuffer) {
        return;
    }

    if ((uint64_t)fb->base != (uint64_t)fb->backbuffer) {
        fb_fast_memcpy(fb->base, fb->backbuffer, fb->backbuffer_length);
    }
}

/**
 * @brief Clean up framebuffer resources
 *
 * @param fb Framebuffer to clean up
 */
void fb_cleanup(FRAMEBUFFER *fb) {
    if (!fb) {
        return;
    }

    if (fb->backbuffer && (uint64_t)fb->base != (uint64_t)fb->backbuffer) {
        kfree(fb->backbuffer);
        fb->backbuffer = NULL;
    }

    if (fb->background_buffer) {
        kfree(fb->background_buffer);
        fb->background_buffer = NULL;
    }

    if (fb->swapbuffer) {
        kfree(fb->swapbuffer);
        fb->swapbuffer = NULL;
    }
}