#pragma once

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

/* printf() states */
#define PRINTF_STATE_NORMAL (0)
#define PRINTF_STATE_LENGTH (1)
#define PRINTF_STATE_LENGTH_SHORT (2)
#define PRINTF_STATE_LENGTH_LONG (3)
#define PRINTF_STATE_SPECIFIER (4)

#define PRINTF_LENGTH_DEFAULT (0)
#define PRINTF_LENGTH_SHORT_SHORT (1)
#define PRINTF_LENGTH_SHORT (2)
#define PRINTF_LENGTH_LONG (3)
#define PRINTF_LENGTH_LONG_LONG (4)

#define SCREEN_WIDTH (80)
#define SCREEN_HEIGHT (25)
#define DEFAULT_COLOR (0x7)

void clear_screen();
void putc(char);
void puts(const char *);
void printf(const char *, ...);
void print_buffer(const char *, const void *, uint32_t);
