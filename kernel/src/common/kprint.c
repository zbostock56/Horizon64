/**
 * @file kprint.c
 * @author Zack Bostock
 * @brief Internal logging functionality
 * @verbatim
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <common/kprint.h>

/**
 * @brief Dumps character to 0xE9 COM port
 *
 * @param c Character to dump
 */
void kputc(char c) {
  __asm__ __volatile__("outb %0, %1" ::"a"(c), "Nd"(0xe9) : "memory");
}

// void klog_lock() {
//   LOCK_LOCK(&klog_shackle);
// }

// void klog_unlock() {
//   UNLOCK_LOCK(&klog_shackle);
// }

// void klog_init() {
//   klog_lock();

//   klog.start = 0;
//   klog.end = 0;

//   klog_unlock();
// }

/**
 * @brief Prints a message to the log.
 *
 * @param msg Message to log
 */
void kprint(const char *msg) {
  for (size_t i = 0; msg[i]; i++) {
    kputc(msg[i]);
  }
}

/**
 * @brief Prints string to the log
 *
 * @param msg String to log
 */
void kputs(const char *msg) {
  kprint(msg);
  kputc('\n');
}

/**
 * @brief Internal helper function to print numbers.
 *
 * @param num number to print as hex to the screen.
 */
static void kprint_hex(size_t num, uint32_t width) {
  int i;
  char buf[17];
  if (!num) {
    kprint("0x0");
    return;
  }

  buf[16] = 0;
  if (width != 0) {
    for (i = 15; num && i >= (16 - width); i--) {
      buf[i] = CONVERSION_TABLE[num % 16];
      num /= 16;
    }
  } else {
    for (i = 15; num; i--) {
      buf[i] = CONVERSION_TABLE[num % 16];
      num /= 16;
    }
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

/**
 * @brief printf helper to put in log
 *
 * @param format Format string
 * @param ... variatic arguments
 */
void kprintf(const char *format, ...) {
  va_list argp;
  va_start(argp, format);

  while (*format != '\0') {
    if (*format == '%') {
      format++;
      uint32_t arg_width = 0;
      if (((*format) >= '0') && ((*format) <= '9')) {
        arg_width *= 10;
        arg_width += *format - '0';
        format++;
      }
      if (*format == 'x') {
        kprint_hex(va_arg(argp, size_t), arg_width);
      } else if (*format == 'd') {
        kprint_dec(va_arg(argp, size_t));
      } else if (*format == 's') {
        kprint(va_arg(argp, char *));
      } else if (*format == 'c') {
        kputc(va_arg(argp, int));
      }
    } else {
      kputc(*format);
    }
    format++;
  }
  va_end(argp);
}

/**
 * @brief Logging internal implementation
 *
 * @param level Logging level
 * @param format Format string
 * @param ... variatic arguments
 */
void klog_implementation(int level, const char *format, ...) {
  switch (level) {
    case LEVEL_LOG:
      kprintf("\e[32m[INFO] \e[0m ");
      break;
    case LEVEL_ERROR:
      kprintf("\e[31m[ERROR]\e[0m ");
      break;
    case LEVEL_DEBUG:
      kprintf("\e[34m[DEBUG]\e[0m ");
      break;
  }

  va_list argp;
  va_start(argp, format);
  kprintf(format, argp);
  va_end(argp);
}
