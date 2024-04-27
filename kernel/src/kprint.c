#include <kprint.h>

void kputc(char c) {
  __asm__ __volatile__("outb %0, %1" ::"a"(c), "Nd"(0xe9) : "memory");
}

void kprint(const char *msg) {
  for (size_t i = 0; msg[i]; i++) {
    kputc(msg[i]);
  }
}

void kputs(const char *msg) {
  kprint(msg);
  kputc('\n');
}

static void kprint_hex(size_t num) {
  int i;
  char buf[17];
  if (!num) {
    kprint("0x0");
    return;
  }

  buf[16] = 0;

  for (i = 15; num; i--) {
    buf[i] = CONVERSION_TABLE[num % 16];
    num /= 16;
  }

  i++;
  kprint("0x");
  kprint(&buf[i]);
}

static void kprint_dec(size_t num) {
  int i;
  char buf[21] = {0};

  if (!num) {
    kputc('0');
    return;
  }

  for (i = 19; num; i--) {
    buf[i] = (num % 10) + 0x30;
    num /= 10;
  }
  i++;
  kprint(buf + i);
}

void kprintf(const char *format, ...) {
  va_list argp;
  va_start(argp, format);

  while (*format != '\0') {
    if (*format == '%') {
      format++;
      if (*format == 'x') {
        kprint_hex(va_arg(argp, size_t));
      } else if (*format == 'd') {
        kprint_dec(va_arg(argp, size_t));
      } else if (*format == 's') {
        kprint(va_arg(argp, char *));
      }
    } else {
      kputc(*format);
    }
    format++;
  }
  va_end(argp);
}