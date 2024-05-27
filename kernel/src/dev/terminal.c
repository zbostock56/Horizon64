#include <dev/terminal.h>

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
  term.width = fb.width / PSF1_FONT_WIDTH;
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

  if ((t->cursor_pos.x + 1) >= t->width) {
    t->cursor_pos.x = TERMINAL_LEFT_OFFSET;
    t->cursor_pos.y++;
    return;
  }
  if (t->cursor_pos.y >= t->height &&
      !(t->cursor_pos.y == t->height &&
      t->cursor_pos.x == 0)) {
    terminal_scroll(t);
    t->cursor_pos.y--;
    return;
  }
  t->cursor_pos.x++;
}

void terminal_pop_cursor(TERMINAL *t) {
  if (!t) {
    kprintf("Null terminal!\n");
    return;
  }

  if (--t->cursor_pos.x < 1) {
    t->cursor_pos.x++;
  }
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

  /* Backspace */
  if (c == 8) {
    terminal_pop_cursor(t);
    fb_putc(&t->framebuffer, t->cursor_pos.x * PSF1_FONT_WIDTH,
          t->cursor_pos.y * font.header->character_size, t->foreground_color,
          t->background_color, ' ');
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

static void terminal_hex(TERMINAL *t, size_t num) {
  int i;
  char buf[17];
  if (!num) {
    terminal_puts(t, "0x0");
    return;
  }

  buf[16] = 0;

  for (i = 15; num; i--) {
    buf[i] = CONVERSION_TABLE[num % 16];
    num /= 16;
  }

  i++;
  terminal_puts(t, "0x");
  terminal_puts(t, &buf[i]);
}

static void terminal_dec(TERMINAL *t, size_t num) {
  int i;
  char buf[21] = {0};

  if (!num) {
    terminal_putc(t, '0');
    return;
  }

  for (i = 19; num; i--) {
    buf[i] = (num % 10) + 0x30;
    num /= 10;
  }
  i++;
  terminal_puts(t, buf + i);
}

void terminal_printf(TERMINAL *t, const char *format, ...) {
  va_list argp;
  va_start(argp, format);

  while (*format != '\0') {
    if (*format == '%') {
      format++;
      if (*format == 'x') {
        terminal_hex(t, va_arg(argp, size_t));
      } else if (*format == 'd') {
        terminal_dec(t, va_arg(argp, size_t));
      } else if (*format == 's') {
        terminal_puts(t, va_arg(argp, char *));
      }
    } else {
      terminal_putc(t, *format);
    }
    format++;
  }
  va_end(argp);
}

void terminal_clear(TERMINAL *t) {
  for (size_t y = 0; y < t->framebuffer.height; y++) {
    for (size_t x = 0; x < t->framebuffer.width; x++) {
      fb_putpixel(&(t->framebuffer), x, y, t->background_color);
    }
  }

  t->cursor_pos.x = 0;
  t->cursor_pos.y = 0;
}

void terminal_scroll(TERMINAL *t) {
  const uint8_t char_size = font.header->character_size;

  for (size_t y = 0; y < (t->cursor_pos.y - 1) * char_size; y++) {
      for (size_t x = 0; x < t->framebuffer.width; x++) {
          uint32_t c = fb_getpixel(&(t->framebuffer), x, y + char_size);
          fb_putpixel(&(t->framebuffer), x, y, c);
      }
    }

    size_t y = (t->cursor_pos.y - 1) * char_size;
    for (; y < t->framebuffer.height; y++) {
        for (size_t x = 0; x < t->framebuffer.width; x++) {
            fb_putpixel(&(t->framebuffer), x, y, t->background_color);
        }
    }
}