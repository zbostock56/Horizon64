/**
 * @file kprint.c
 * @author Zack Bostock
 * @brief Internal logging functionality
 * @verbatim
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <kconfig.h>

#include <common/kprint.h>
#include <common/lock.h>

#include <sys/smp.h>
#include <sys/acpi/hpet.h>
#include <sys/cmos.h>

#include <dev/terminal.h>

#include <proc/ctxsw.h>

/**
 * @brief Each log is correlated with a terminal structure
 */
static KLOG klog_info = {0};
static KLOG klog_cli = {0};

static LOCK klog_info_lock = {0};

/**
 * @brief Debugging variables used to keep metrics
 */
static uint64_t klog_clear_times = 0;
static uint64_t klog_refresh_times = 0;
static uint64_t klog_putchar_times = 0;

extern int timer_enabled;

/**
 * @brief Main initialization function for kernel logging
 */
void klog_init() {
    LOCK_LOCK(&klog_info_lock);

    memset(klog_info.buffer, 0, KLOG_BUFFER_SIZE);
    memset(klog_cli.buffer, 0, KLOG_BUFFER_SIZE);

    klog_info.start = 0;
    klog_info.end = 0;

    klog_cli.start = 0;
    klog_cli.end = 0;

    UNLOCK_LOCK(&klog_info_lock);
}

/**
 * @brief Used externally to lock the klog lock
 *
 */
void klog_lock() {
    LOCK_LOCK(&klog_info_lock);
}

/**
 * @brief Used externally to unlock the klog lock
 */
void klog_unlock() {
    UNLOCK_LOCK(&klog_info_lock);
}

/**
 * @brief Dumps character to 0xE9 COM port
 *
 * @param mode Terminal mode
 * @param c Character to dump
 */
void kputc(TERM_MODE mode, uint8_t c) {
    __asm__ __volatile__("outb %0, %1" ::"a"(c), "Nd"(0xe9) : "memory");
    KLOG *k = ((mode == TERM_MODE_INFO)) ? &klog_info : &klog_cli;

    k->buffer[k->end] = c;
    k->end++;
    if (k->end >= KLOG_BUFFER_SIZE) {
        k->end = 0;
    }

    if (k->end == k->start) {
        k->start++;
    }

    if (k->start >= KLOG_BUFFER_SIZE) {
        k->start = 0;
    }

    terminal_putc(mode, c);
    klog_putchar_times++;

}

/**
 * @brief Calls kputc for an entire string
 *
 * @param mode Terminal mode
 * @param s String to print
 * @param width Number of characters to print
 */
void kputs(TERM_MODE mode, const char *s, int width) {
    int i = 0;
    for (; s[i] != '\0'; i++) {
        kputc(mode, s[i]);
    }

    /* If width is specified and the number of characters printed is less than */
    /* the width requested, continue printing spaces until the request is met  */
    if (width > 0) {
        for (; i < width; i++) {
            kputc(mode, ' ');
        }
    }
}

/**
 * @brief Helper to clear the terminal related to the current log
 *
 * @param mode Terminal mode
 */
void klog_refresh(TERM_MODE mode) {
    if (terminal_need_redraw()) {
        KLOG *k = ((mode == TERM_MODE_INFO)) ? &klog_info : &klog_cli;

        terminal_clear(mode);

        /* Note that the string ends at k->end - 1 */
        int i = k->start;
        while (TRUE) {
            if (i >= KLOG_BUFFER_SIZE) {
                i = 0;
            }
            if (k->end >= k->start) {
                if (i >= k->end) {
                    break;
                }
            } else {
                if (i >= k->end && i < k->start) {
                    break;
                }
            }

            terminal_putc(mode, k->buffer[i]);
            klog_putchar_times++;
            i++;
        }
        klog_clear_times++;
        terminal_set_redraw(FALSE);
    }

    terminal_refresh(mode);
    klog_refresh_times++;
}

/**
 * @brief Internal helper function to print numbers.
 *
 * @param mode Terminal mode
 * @param num number to print as hex to the screen.
 * @param width Number of characters to print
 */
static void kprint_hex(TERM_MODE mode, uint64_t num, uint32_t width) {
    if (!num) {
        kputs(mode, "0x0", width);
        return;
    }

    kputs(mode, "0x", 0);
    int k = 0;
    for (int i = 60; i >= 0; i -= 4) {
        k++;
        if (width > 0 && (k + width) <= 16) {
            continue;
        }

        uint64_t digit = (num >> i) & 0xF;
        kputc(mode, (digit <= 9) ? (digit + '0') : (digit - 10 + 'A'));
    }
}

/**
 * @brief Helper for printing binary to the screen
 *
 * @param mode Terminal mode
 * @param num Number to print in binary
 * @param width Output width
 * @param mid_blank Do we want binaries to be separated by four digits?
 */
static void kprint_bin(TERM_MODE mode, uint64_t num, uint32_t width, uint8_t mid_blank) {
    if (!num) {
        kputs(mode, "0b0", width);
        return;
    }

    kputs(mode, "0b", 0);
    int k = 0;
    for (int i = 63; i >= 0; i--) {
        k++;
        if (width > 0 && (i + width) <= 64) {
            continue;
        }

        uint64_t digit = (num >> i) & 0x1;
        kputc(mode, (digit == 0) ? '0' : '1');
        /* If we want to have spaces between each set of four in binary */
        if ((i % 4 == 0) && i > 0 && mid_blank) {
            kputc(mode, ' ');
        }
    }
}

/**
 * @brief Helper method to print out integers
 *
 * @param mode Terminal mode
 * @param num Number to print
 * @param width Width of characters to print
 * @param zero_filling TRUE to zero fill, FALSE otherwise
 */
static void kprint_int(TERM_MODE mode, int64_t num, uint32_t width,
                       uint8_t zero_filling) {
    int64_t val = num;
    uint32_t val_width = 1;
    uint32_t zero_width = 0;
    uint64_t i = 9;

    if (num < 0) {
        val = -val;
    }

    while (val > (int64_t) i && i < UINT64_MAX) {
        val_width += 1;
        i *= 10;
        i += 9;
    }

    if (num < 0) {
        val_width -= 1;
        kputc(mode, '-');
        num = -num;
    }

    while (zero_width + val_width < width) {
        kputc(mode, zero_filling ? '0' : ' ');
        zero_width++;
    }

    if (num == 0) {
        kputc(mode, '0');
    }

    size_t div = 1;
    size_t temp = num;
    while (temp > 0) {
        temp /= 10;
        div *= 10;
    }

    while (div >= 10) {
        uint8_t digit = ((num % div) - (num % (div / 10))) / (div / 10);
        div /= 10;
        kputc (mode, digit + '0');
    }
}

/**
 * @brief Main core klog system
 *
 * @param mode Terminal mode
 * @param s String to print
 * @param args Variadic arguments
 */
static void klog_vprintf_core(TERM_MODE mode, const char *s, va_list args) {
    for (size_t i = 0; s[i] != '\0'; i++) {
        switch (s[i]) {
            case '%': {
                uint32_t arg_width = 0;
                uint8_t zero_filling = FALSE;
                if (s[i + 1] == '0') {
                    zero_filling = TRUE;
                }
                while (s[i + 1] >= '0' && s[i + 1] <= '9') {
                    arg_width *= 10;
                    arg_width += s[i + 1] - '0';
                    i++;
                }
                switch (s[i + 1]) {
                    case '%':
                        kputc(mode, '%');
                        break;
                    case 'd':
                        kprint_int(mode, va_arg(args, int64_t), arg_width, zero_filling);
                        break;
                    case 'x':
                        kprint_hex(mode, va_arg(args, uint64_t), arg_width);
                        break;
                    case 'b':
                        kprint_bin(mode, va_arg(args, uint64_t), arg_width, !zero_filling);
                        break;
                    case 's':
                        kputs(mode, va_arg(args, const char *), arg_width);
                        break;
                    case 'c':
                        kputc(mode, va_arg(args, int));
                        break;
                    case 't':
                        kputs(mode, va_arg(args, int) ? "true" : "false", 0);
                        break;
                }
                i++;
            }
            break;
            default:
                kputc(mode, s[i]);
        }
    }
}

/**
 * @brief Wrapper around the core printing functionailty
 *
 * @param mode Terminal mode
 * @param s String to print
 * @param ... Variadic arguments
 */
static void klog_vprintf_wrapper(TERM_MODE mode, const char *s, ...) {
    va_list args;
    va_start(args, s);
    klog_vprintf_core(mode, s, args);
    va_end(args);
}

/**
 * @brief Virtual printf wrapper
 *
 * @param level Printing level
 * @param s String to print
 * @param ... Variadic arguments
 */
void klog_vprintf(uint8_t level, const char *s, ...) {
    #ifndef ENABLE_KLOG_DEBUG
    if (level <= KLOG_LVL_DEBUG) {
        return;
    }
    #else
    if (level <= KLOG_LVL_VERBOSE) {
        return;
    }
    #endif

    if (level < KLOG_LVL_UNKNOWN) {
        LOCK_LOCK(&klog_info_lock);
        if (timer_enabled && level != KLOG_LVL_NONE) {
            CPU *cpu = smp_get_curr_cpu(NO_FORCE_GET_CPU);
            uint64_t nanos = hpet_get_nanos();
            uint64_t nsecs = NANOS_TO_SECONDS(nanos);
            uint64_t nms = NANOS_TO_MILLIS(nanos) % 1000;
            uint64_t bootsecs = cmos_get_boot_time_seconds();

            STD_TIME t = {0};
            seconds_to_std_time(bootsecs + nsecs, &t);

            /* TODO: Change all this to sprintf to save calls to vprintf */

            klog_vprintf_wrapper(TERM_MODE_INFO, "%04d-%02d-%02d %02d:%02d:%02d %03d ",
                                1900 + t.year, t.month + 1, t.dom, t.hours, t.minutes,
                                t.seconds, nms);
            if (cpu) {
                klog_vprintf_wrapper(TERM_MODE_INFO, "%02d", cpu->cpu_id);
            } else {
                klog_vprintf_wrapper(TERM_MODE_INFO, "--");
            }

            PROCESS *pcurr = sched_get_curr_proc();
            if (pcurr) {
                klog_vprintf_wrapper(TERM_MODE_INFO, "-%03d ", pcurr->id);
            } else {
                klog_vprintf_wrapper(TERM_MODE_INFO, "---- ");
            }
        } else if (level != KLOG_LVL_NONE) {
            klog_vprintf_wrapper(TERM_MODE_INFO, "0000-00-00 00:00:00 000 ------ ");
        }

        switch (level) {
            case KLOG_LVL_VERBOSE:
                klog_vprintf_wrapper(TERM_MODE_INFO, "\e[34m[VERBOSE] \e[0m ");
                break;
            case KLOG_LVL_DEBUG:
                klog_vprintf_wrapper(TERM_MODE_INFO, "\e[34m[DEBUG]\e[0m ");
                break;
            case KLOG_LVL_INFO:
                klog_vprintf_wrapper(TERM_MODE_INFO, "\e[32m[INFO ]\e[0m ");
                break;
            case KLOG_LVL_WARN:
                klog_vprintf_wrapper(TERM_MODE_INFO, "\e[33m[WARN ]\e[0m ");
                break;
            case KLOG_LVL_ERROR:
                klog_vprintf_wrapper(TERM_MODE_INFO, "\e[31m[ERROR]\e[0m ");
                break;
            case KLOG_LVL_TAB:
                klog_vprintf_wrapper(TERM_MODE_INFO, "\t");
                break;
            case KLOG_LVL_SRTUP:
                klog_vprintf_wrapper(TERM_MODE_INFO, "\e[93m[SRTUP]\e[0m ");
                break;
            case KLOG_LVL_NONE:
            default:
                break;
        }

        va_list args;
        va_start(args, s);
        klog_vprintf_core(TERM_MODE_INFO, s, args);
        va_end(args);

        klog_refresh(TERM_MODE_INFO);
        if (level < KLOG_LVL_UNKNOWN) {
            UNLOCK_LOCK(&klog_info_lock);
        }
    }
}

/**
 * @brief Main kernel printing function
 *
 * @param s Format string
 * @param ... Variadic arguments
 */
void kprintf(const char *s, ...) {
    LOCK_LOCK(&klog_info_lock);

    va_list args;
    va_start(args, s);
    klog_vprintf_core(TERM_MODE_TERM, s, args);
    va_end(args);

    klog_refresh(TERM_MODE_TERM);
    UNLOCK_LOCK(&klog_info_lock);
}