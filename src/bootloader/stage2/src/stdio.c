#include "../include/stdio.h"
#include "../include/x86.h"

uint8_t *g_screen_buffer = (uint8_t *) 0xB8000;
int g_screenx = 0;
int g_screeny = 0;

void putchr(int x, int y, char c) {
    g_screen_buffer[2 * (y * SCREEN_WIDTH + x)] = c;
}

void putcolor(int x, int y, uint8_t color) {
    g_screen_buffer[2 * (y * SCREEN_WIDTH + x) + 1] = color;
}

char getchr(int x, int y) {
    return g_screen_buffer[2 * (y * SCREEN_WIDTH + x)];
}

uint8_t getcolor(int x, int y) {
    return g_screen_buffer[2 * (y * SCREEN_WIDTH + x) + 1];
}

/*
    Sets hardware cursor
*/
void setcursor(int x, int y) {
    int pos = y * SCREEN_WIDTH + x;
    x86_outb(0x3D4, 0x0F);
    x86_outb(0x3D5, (uint8_t)(pos & 0xFF));
    x86_outb(0x3D4, 0x0E);
    x86_outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

/*
    Nulls out all characters on the screen
*/
void clear_screen() {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            putchr(x, y, '\0');
            putcolor(x, y, DEFAULT_COLOR);
        }
    }
    g_screenx = 0;
    g_screeny = 0;
    setcursor(g_screenx, g_screeny);
}

/*
    Moves all characters up/down the screen the number of
    time denoted from the argument
*/
void scrollback(int lines) {
    /* Move characters up/down */
    for (int y = lines; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            putchr(x, y - lines, getchr(x, y));
            putcolor(x, y - lines, getchr(x, y));
        }
    }
    /* Clear all spaces that once had characters */
    for (int y = SCREEN_HEIGHT - lines; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            putchr(x, y, '\0');
            putcolor(x, y, DEFAULT_COLOR);
        }
    }
    g_screeny -= lines;
}

/*
    Prints one character to the screen
*/
void putc(char c) {
    switch (c) {
        case '\n':
            g_screenx = 0;
            g_screeny++;
            break;
        case '\t':
            for (int i = 0; i < 4 - (g_screenx % 4); i++) {
                putc(' ');
            }
            break;
        case '\r':
            g_screenx = 0;
            break;
        default:
            putchr(g_screenx++, g_screeny, c);
            break;
    }
    /* Update location of for next character */
    if (g_screenx >= SCREEN_WIDTH) {
        g_screeny++;
        g_screenx = 0;
    }
    if (g_screeny >= SCREEN_HEIGHT) {
        scrollback(1);
    }
    setcursor(g_screenx, g_screeny);
}

/*
    Prints a character string to the screen
*/
void puts(const char *str) {
    while (*str) {
        putc(*str);
        str++;
    }
}

void printf_unsigned(unsigned long long number, int radix) {
    char buffer[32];
    int pos = 0;
    const char hex_chars[] = "0123456789abcdef";

    /* Convert number to ASCII */
    do {
        unsigned long long rem = number % radix;
        number /= radix;
        buffer[pos++] = hex_chars[rem];
    } while (number > 0);

    /* Print in reverse order */
    while (--pos >= 0) {
        putc(buffer[pos]);
    }
}

void printf_signed(long long number, int radix) {
    if (number < 0) {
        putc('-');
        printf_unsigned(-number, radix);
    } else {
        printf_unsigned(number, radix);
    }
}

/*
    Handles printing formatted strings to the screen
*/
void printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    int state = PRINTF_STATE_NORMAL;
    int length = PRINTF_LENGTH_DEFAULT;
    int radix = 10;
    int sign = false;
    int number = false;

    while (*fmt) {
        switch (state) {
            case PRINTF_STATE_NORMAL:
                switch (*fmt) {
                case '%':
                    state = PRINTF_STATE_LENGTH;
                    break;
                default:
                    putc(*fmt);
                    break;
                }
            break;
            case PRINTF_STATE_LENGTH:
                switch (*fmt) {
                case 'h':
                    length = PRINTF_LENGTH_SHORT;
                    state = PRINTF_STATE_LENGTH_SHORT;
                    break;
                case 'l':
                    length = PRINTF_LENGTH_LONG;
                    state = PRINTF_STATE_LENGTH_LONG;
                    break;
                default:
                    goto PRINTF_STATE_SPECIFIER_;
                    break;
                }
            break;
            case PRINTF_STATE_LENGTH_SHORT:
                if (*fmt == 'h') {
                    length = PRINTF_LENGTH_SHORT_SHORT;
                    state = PRINTF_STATE_SPECIFIER;
                } else {
                    goto PRINTF_STATE_SPECIFIER_;
                }
            break;
            case PRINTF_STATE_LENGTH_LONG:
                if (*fmt == 'l') {
                    length = PRINTF_LENGTH_LONG_LONG;
                    state = PRINTF_STATE_SPECIFIER;
                } else {
                    goto PRINTF_STATE_SPECIFIER_;
                }
            break;
            case PRINTF_STATE_SPECIFIER:
                PRINTF_STATE_SPECIFIER_:
                    switch (*fmt) {
                        case 'c':
                            putc((char) va_arg(args, int));
                            break;
                        case 's':
                            puts(va_arg(args, const char *));
                            break;
                        case '%':
                            putc('%');
                            break;
                        case 'd':
                        case 'i':
                            radix = 10;
                            sign = true;
                            number = true;
                            break;
                        case 'u':
                            radix = 10;
                            sign = false;
                            number = true;
                            break;
                        case 'X':
                        case 'x':
                        case 'p':
                            radix = 16;
                            sign = false;
                            number = true;
                            break;
                        case 'o':
                            radix = 8;
                            sign = false;
                            number = true;
                            break;
                        default:
                            puts("\nInvalid specifier\n");
                            break;

                    }

                    /* Handle number case */
                    if (number) {
                        if (sign) {
                            switch (length) {
                                case PRINTF_LENGTH_SHORT_SHORT:
                                case PRINTF_LENGTH_SHORT:
                                case PRINTF_LENGTH_DEFAULT:
                                    printf_signed(va_arg(args, int), radix);
                                    break;
                                case PRINTF_LENGTH_LONG:
                                    printf_signed(va_arg(args, long), radix);
                                    break;
                                case PRINTF_LENGTH_LONG_LONG:
                                    printf_signed(va_arg(args, long long), radix);
                                break;
                            }
                        } else {
                            switch (length) {
                                case PRINTF_LENGTH_SHORT_SHORT:
                                case PRINTF_LENGTH_SHORT:
                                case PRINTF_LENGTH_DEFAULT:
                                    printf_unsigned(va_arg(args, int), radix);
                                    break;
                                case PRINTF_LENGTH_LONG:
                                    printf_unsigned(va_arg(args, long), radix);
                                    break;
                                case PRINTF_LENGTH_LONG_LONG:
                                    printf_unsigned(va_arg(args, long long), radix);
                                break;
                            }
                        }
                    }

                    /* Reset state for next interation */
                    state = PRINTF_STATE_NORMAL;
                    length = PRINTF_LENGTH_DEFAULT;
                    radix = 10;
                    sign = false;
                    number = false;
                    break;
        }
        fmt++;
    }
    va_end(args);
}
