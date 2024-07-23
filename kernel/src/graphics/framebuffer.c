/**
 * @file framebuffer.c
 * @author Zack Bostock
 * @brief Helpers and initalization for the screen framebuffer
 * @verbatim
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <graphics/framebuffer.h>

/**
 * @brief Initialization of a framebuffer
 *
 * @param req Request for framebuffer from bootloader
 */
void fb_init(struct limine_framebuffer_request req) {

  if (!req.response || req.response->framebuffer_count < 1) {
    kprintf("Framebuffer either no response or not correct count!\n");
    halt();
  }

  struct limine_framebuffer *fb = req.response->framebuffers[0];
  if (fb != NULL) {
    initial_fb.base = fb->address;
    initial_fb.backbuffer = fb->address;
    initial_fb.height = fb->height;
    initial_fb.width = fb->width;
    initial_fb.pitch = fb->pitch;
    initial_fb.bpp = fb->bpp;
    // initial_fb.swapbuffer = malloc(fb->width * fb->height * 4);
  }

  kprintf("Successfully initialized framebuffer\n");
}

/*
  TODO: Update when memory allocation is finished. Need to be able
        to write to the backbuffer and swap buffers to the front buffer
        in order to properly support double-buffering.
*/

/**
 * @brief Helper for changing the color of a pixel on the screen.
 *
 * @param fb Framebuffer to change
 * @param x X-coordinate on the screen
 * @param y Y-coordinate on the screen
 * @param color Color to change to
 */
void fb_putpixel(FRAMEBUFFER *fb, uint32_t x, uint32_t y, uint32_t color) {
  // if ((uint64_t) fb->base == (uint64_t) fb->backbuffer) {
  //     kprintf("base == backbuffer!\n");
  //     return;
  // }

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
  // if ((uint64_t) fb->base == (uint64_t) fb->backbuffer) {
  //     kprintf("base == backbuffer!\n");
  //     return;
  // }

    if ((fb->pitch * y + x * 4) < fb->pitch * fb->height) {
        return ((uint32_t *)(fb->backbuffer + (fb->pitch * y)))[x];
    } else {
        return 0;
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
 */
void fb_putc(FRAMEBUFFER *fb, uint32_t x, uint32_t y, uint32_t fgcolor,
              uint32_t bgcolor, uint8_t ch) {
  // if ((uint64_t) fb->base == (uint64_t) fb->backbuffer) {
  //     kprintf("base == backbuffer!\n");
  //     return;
  // }

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
    uint64_t len = fb->pitch * fb->height;
    uint32_t *src = (uint32_t *)fb->backbuffer;
    uint32_t *dst = (uint32_t *)fb->swapbuffer;
    size_t pt_num = len / 4;
    for (size_t i = 0; i < pt_num; i++) {
      if (src[i] != DEFAULT_BG) {
        dst[i] = src[i];
      }
    }
    memcpy(fb->base, fb->swapbuffer, len);
  }
}

/**
 * @brief Helper to clear the screen
 *
 * @param fb Framebuffer to clear
 */
void fb_clear_screen(FRAMEBUFFER *fb) {
  // TODO
}
