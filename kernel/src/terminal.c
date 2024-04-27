#include <terminal.h>

/*
  Initializes a framebuffer for the terminal and
  sets basic information about the framebuffer and the
  terminal variables to support text output
*/
void init_terminal(FRAMEBUFFER fb) {
  /* TODO: pass as pointer once memory allocation finished */
  // if (!fb) {
  //   kprintf("NULL framebuffer when trying to init terminal!\n");
  //   return;
  // }
  term.framebuffer = fb;
  term.background_color = COLOR_BLACK;
  term.foreground_color = COLOR_WHITE;
  /* TODO: Edit to support multiple fonts */
  term.height = fb.height / font.header->character_size;
  term.height = fb.width / PSF1_FONT_WIDTH;
  term.cursor_pos = IVEC2_ONE;
}

void terminal_set_foreground(TERMINAL *t, FRAMEBUFFER_COLORS color) {
  if (!t) {
    kprintf("Null terminal!\n");
    return;
  }
  t->foreground_color = color;
}

void terminal_set_background(TERMINAL *t, FRAMEBUFFER_COLORS color) {
  if (!t) {
    kprintf("Null terminal!\n");
    return;
  }
  t->background_color = color;
}

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

void terminal_push_cursor(TERMINAL *t) {
  if (!t) {
    kprintf("Null terminal!\n");
    return;
  }

  /* TODO: t->width seems to be zero, look into this */

  // if ((t->cursor_pos.x + 1) > t->width) {
  //   t->cursor_pos.x = 0;
  //   t->cursor_pos.y++;
  //   return;
  // }
  t->cursor_pos.x++;
}

void terminal_putc(TERMINAL *t, uint8_t c) {
  if (!t) {
    kprintf("Null terminal!\n");
    return;
  }
  if (c == '\n') {
    t->cursor_pos.y++;
    t->cursor_pos.x = 1;
    return;
  }
  fb_putc(&t->framebuffer, t->cursor_pos.x * PSF1_FONT_WIDTH,
          t->cursor_pos.y * font.header->character_size, t->foreground_color,
          t->background_color, c);
  terminal_push_cursor(t);
}

void terminal_puts(TERMINAL *t, const char *s) {
  for (size_t i = 0; s[i]; i++) {
    terminal_putc(t, s[i]);
  }
}