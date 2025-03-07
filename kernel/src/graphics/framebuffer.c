/**
 * @file framebuffer.c
 * @author Zack Bostock
 * @brief Helpers and initalization for the screen framebuffer
 * @verbatim
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <common/kmalloc.h>
#include <common/string.h>
#include <common/memory.h>
#include <common/kprint.h>

#include <sys/asm.h>

#include <graphics/framebuffer.h>

/**
 * @brief Initialization of a framebuffer
 *
 * @param fb Framebuffer to initialize
 * @param f Limine framebuffer struct
 */
void fb_init(FRAMEBUFFER *fb, struct limine_framebuffer *f) {
    klogs("INIT FRAMEBUFFER: starting...\n");

    if (!f) {
        if ((uint64_t) fb->base == (uint64_t) fb->backbuffer) {
            fb->backbuffer = kmalloc(fb->backbuffer_length);
            memcpy(fb->backbuffer, fb->base, fb->backbuffer_length);
        }
        return;
    }

    /* Copy over the information from the Limine structure */
    fb->base = f->address;
    fb->swapbuffer = NULL;
    fb->backbuffer = fb->base;
    fb->width = f->width;
    fb->height = f->height;
    fb->pitch = f->pitch;

    /* Set up the background buffer */
    fb->backbuffer_length = fb->height * fb->pitch;
    fb->background_buffer = NULL;

    /* Set the backbuffer to the default background color */
    for (uint32_t x = 0; x < fb->width; x++) {
        for (uint32_t y = 0; y < fb->height; y++) {
            fb_putpixel(fb, x, y, DEFAULT_BG);
        }
    }

    /* Now, copy the backbuffer into the into swapbuffer, then to front buffer */
    /* so the screen is set to the background and the buffers are set up.      */
    fb_refresh(fb);
    klogs("INIT FRAMEBUFFER: finished...\n");
}

/**
 * @brief Helper for changing the color of a pixel on the screen.
 *
 * @param fb Framebuffer to change
 * @param x X-coordinate on the screen
 * @param y Y-coordinate on the screen
 * @param color Color to change to
 */
void fb_putpixel(FRAMEBUFFER *fb, uint32_t x, uint32_t y, uint32_t color) {
    if ((uint64_t) fb->base == (uint64_t) fb->backbuffer) {
        return;
    }

    if ((fb->pitch * y + x * 4) < fb->pitch * fb->height) {
        ((uint32_t *)(fb->backbuffer + (fb->pitch * y)))[x] = color;
    }
}

/**
 * @brief Helper to get a pixels color from the screen
 *
 * @param fb Framebuffer to view
 * @param x X-coordinate on the screen
 * @param y Y-coordinate on the screen
 * @return uint32_t Color value of the pixel
 */
uint32_t fb_getpixel(FRAMEBUFFER *fb, uint32_t x, uint32_t y) {
    if ((uint64_t) fb->base == (uint64_t) fb->backbuffer) {
        return 0;
    }

    if ((fb->pitch * y + x * 4) < fb->pitch * fb->height) {
        return ((uint32_t *)(fb->backbuffer + (fb->pitch * y)))[x];
    } else {
        return 0;
    }
}

void fb_draw_characters(FRAMEBUFFER *fb, uint32_t fg, uint32_t bg,
                        const char *str) {
    if ((uint64_t) fb->base == (uint64_t) fb->backbuffer) {
        return;
    }

    size_t str_len = strlen(str);
    uint32_t x = (fb->width - str_len * 8 * 6) / 2;
    uint32_t y = (fb->height - 16 * 6) / 2;
    static const uint8_t masks[8] = { 128, 64, 32, 16, 8, 4, 2, 1};

    for (size_t i = 0; i < str_len; i++) {
        uint32_t offset = ((uint32_t) str[i]) * font.header->character_size;
        for (size_t j = 0; j < font.header->character_size; j++) {
            for (size_t k = 0; k < 8; k++) {
                for (size_t m = 1; m < 6; m++) {
                    for (size_t n = 1; n < 6; n++) {
                        uint32_t bg_color = bg;
                        if (fb->background_buffer) {
                            bg = ((uint32_t *)(fb->background_buffer + fb->pitch *
                                              y + x * 4))[0];
                        }
                        uint32_t color = (((uint8_t *) font.glyph_buffer)[offset + j] & masks[k]) ?
                                        fg : bg_color;
                        fb_putpixel(fb, x + (i * 8 + k) * 6 + m, y + j * 6 + n, color);
                    }
                }
            }
        }
    }

}

/**
 * @brief Helper to put a character on the framebuffer.
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
  if ((uint64_t) fb->base == (uint64_t) fb->backbuffer) {
      return;
  }

  /* TODO: Update to use bold fonts */
  (void) is_bold;

  uint32_t offset = ((uint32_t)ch) * font.header->character_size;
  static const uint8_t masks[8] = {128, 64, 32, 16, 8, 4, 2, 1};
  for (size_t i = 0; i < font.header->character_size; i++) {
    for (size_t k = 0; k < 8; k++) {
      if (i < font.header->character_size &&
          ((((uint8_t *)font.glyph_buffer)[offset + i]) & masks[k])) {
        fb_putpixel(fb, x + k, y + i, fgcolor);
      } else {
        fb_putpixel(fb, x + k, y + i, bgcolor);
      }
    }
  }
}

/**
 * @brief Refreshes a framebuffer
 *
 * @param fb Framebuffer struct to refresh
 */
void fb_refresh(FRAMEBUFFER *fb) {
    if ((uint64_t)fb->base != (uint64_t)fb->backbuffer) {
        uint64_t len = fb->backbuffer_length;
        if (fb->background_buffer) {
            /* Copy back buffer into swap buffer */
            memcpy(fb->swapbuffer, fb->backbuffer, len);
            uint32_t *src = (uint32_t *)fb->backbuffer;
            uint32_t *dst = (uint32_t *)fb->swapbuffer;
            size_t pt_num = len / 4;
            for (size_t i = 0; i < pt_num; i++) {
                if (src[i] != DEFAULT_BG) {
                    dst[i] = src[i];
                }
            }
            /* Copy back from swap buffer into front buffer */
            memcpy(fb->base, fb->swapbuffer, len);
        } else {
            memcpy(fb->base, fb->backbuffer, len);
        }
    }
}