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

#include <dev/serial.h>

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

void klog_print_debug_stats() {
    klogd("Clear times: %d\n", klog_clear_times);
    klogd("Refresh times: %d\n", klog_refresh_times);
    klogd("Putc times: %d\n", klog_putchar_times);
}

/**
 * @brief Used externally to lock the klog lock
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
 * @brief Dumps a character to the log buffer.
 *
 * @param k Kernel log to write to
 * @param c Character to dump
 */
static inline void kputc(KLOG *k, uint8_t c) {
    k->buffer[k->end] = c;
    k->end = (k->end + 1) % KLOG_BUFFER_SIZE;
    if (k->end == k->start) {
        k->start = (k->start + 1) % KLOG_BUFFER_SIZE;
    }
}

/**
 * @brief Calls kputc for an entire string
 *
 * @param k Kernel log to write to
 * @param s String to print
 * @param width Number of characters to print (if > 0, pads with spaces)
 */
static inline void kputs(KLOG *k, const char *s, int width) {
    int i = 0;
    while (s[i] != '\0') {
        kputc(k, s[i]);
        i++;
    }
    /* Pad with spaces if needed */
    for (; i < width; i++) {
        kputc(k, ' ');
    }
}

/**
 * @brief Helper to refresh the terminal for the current log
 *
 * This version computes the number of characters to print and then uses two
 * loops if the buffer wraps around.
 *
 * @param mode Terminal mode
 */
void klog_refresh(TERM_MODE mode) {
    if (terminal_need_redraw()) {
        KLOG *k = (mode == TERM_MODE_INFO) ? &klog_info : &klog_cli;
        terminal_clear(mode);

        /* Compute the total number of characters in the buffer */
        unsigned long count;
        if (k->end >= k->start) {
            count = k->end - k->start;
        } else {
            count = (KLOG_BUFFER_SIZE - k->start) + k->end;
        }

        /* Print first segment from k->start to end-of-buffer */
        unsigned long i = k->start;
        unsigned long printed = 0;
        while (printed < count) {
            terminal_putc(mode, k->buffer[i]);
            klog_putchar_times++;
            printed++;
            i = (i + 1) % KLOG_BUFFER_SIZE;
        }
        klog_clear_times++;
        terminal_set_redraw(FALSE);
    }
    terminal_refresh(mode);
    klog_refresh_times++;
}

/**
 * @brief Internal helper function to print numbers in hexadecimal
 *
 * @param k Kernel log to write to
 * @param num Number to print as hex
 * @param width Number of characters to print
 */
static void kprint_hex(KLOG *k, uint64_t num, uint32_t width) {
    if (!num) {
        kputs(k, "0x0", width);
        return;
    }

    kputs(k, "0x", 0);
    int j = 0;
    for (int i = 60; i >= 0; i -= 4) {
        j++;
        if (width > 0 && (j + width) <= 16) {
            continue;
        }
        uint64_t digit = (num >> i) & 0xF;
        kputc(k, (digit <= 9) ? (digit + '0') : (digit - 10 + 'A'));
    }
}

/**
 * @brief Helper for printing a number in binary
 *
 * @param k Kernel log to write to
 * @param num Number to print in binary
 * @param width Output width
 * @param mid_blank Whether to insert a blank between every 4 bits
 */
static void kprint_bin(KLOG *k, uint64_t num, uint32_t width, uint8_t mid_blank) {
    if (!num) {
        kputs(k, "0b0", width);
        return;
    }

    kputs(k, "0b", 0);
    for (int i = 63; i >= 0; i--) {
        /* Optionally skip leading zeros if width is specified */
        if (width > 0 && (i + width) <= 64) {
            continue;
        }
        uint64_t digit = (num >> i) & 0x1;
        kputc(k, (digit == 0) ? '0' : '1');
        if ((i % 4 == 0) && i > 0 && mid_blank) {
            kputc(k, ' ');
        }
    }
}

/**
 * @brief Helper method to print out an integer
 *
 * @param k Kernel log to write to
 * @param num Number to print
 * @param width Width of characters to print
 * @param zero_filling TRUE to zero fill, FALSE otherwise
 */
static void kprint_int(KLOG *k, int64_t num, uint32_t width, uint8_t zero_filling) {
    int64_t val = num;
    uint32_t val_width = 1;
    uint32_t zero_width = 0;
    uint64_t i = 9;

    if (num < 0) {
        val = -val;
    }

    while (val > (int64_t)i && i < UINT64_MAX) {
        val_width++;
        i *= 10;
        i += 9;
    }

    if (num < 0) {
        val_width--;
        kputc(k, '-');
        num = -num;
    }

    while (zero_width + val_width < width) {
        kputc(k, zero_filling ? '0' : ' ');
        zero_width++;
    }

    if (num == 0) {
        kputc(k, '0');
        return;
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
        kputc(k, digit + '0');
    }
}

/**
 * @brief Core function for formatted printing using variadic arguments
 *
 * @param k Kernel log to write to
 * @param s Format string
 * @param args Variadic argument list
 */
static void klog_vprintf_core(KLOG *k, const char *s, va_list args) {
    for (size_t i = 0; s[i] != '\0'; i++) {
        if (s[i] == '%') {
            uint32_t arg_width = 0;
            uint8_t zero_filling = 0;
            i++;
            if (s[i] == '0') {
                zero_filling = 1;
                i++;
            }
            while (s[i] >= '0' && s[i] <= '9') {
                arg_width = arg_width * 10 + (s[i] - '0');
                i++;
            }
            switch (s[i]) {
                case '%':
                    kputc(k, '%');
                    break;
                case 'd':
                    kprint_int(k, va_arg(args, int64_t), arg_width, zero_filling);
                    break;
                case 'x':
                    kprint_hex(k, va_arg(args, uint64_t), arg_width);
                    break;
                case 'b':
                    kprint_bin(k, va_arg(args, uint64_t), arg_width, !zero_filling);
                    break;
                case 's':
                    kputs(k, va_arg(args, const char *), arg_width);
                    break;
                case 'c':
                    kputc(k, (uint8_t)va_arg(args, int));
                    break;
                case 't':
                    kputs(k, va_arg(args, int) ? "true" : "false", 0);
                    break;
                default:
                    /* Unsupported format; print it literally. */
                    kputc(k, s[i]);
            }
        } else {
            kputc(k, s[i]);
        }
    }
}

/**
 * @brief Wrapper around the core printing function.
 *
 * @param k Kernel log to write to
 * @param s Format string
 * @param ... Variadic arguments
 */
static void klog_vprintf_wrapper(KLOG *k, const char *s, ...) {
    va_list args;
    va_start(args, s);
    klog_vprintf_core(k, s, args);
    va_end(args);
}

/**
 * @brief Sends the klog buffered input to its output destination
 *
 * Computes the total number of characters in the log buffer and
 * then uses two loops if the buffer wraps around.
 *
 * @param k Kernel log to read from
 * @param kprintf Determines whether to send to terminal with extra formatting\n
 */
static void klog_send(KLOG *k, uint8_t kprintf) {
    LOCK_LOCK(&klog_info_lock);

    /* Determine number of characters in the log */
    unsigned long count;
    if (k->end >= k->start) {
        count = k->end - k->start;
    } else {
        count = (KLOG_BUFFER_SIZE - k->start) + k->end;
    }

    /* Process and output characters from the log */
    unsigned long i = k->start;
    for (unsigned long printed = 0; printed < count; printed++) {
        /* Copy log entry into CLI buffer (circular buffer update) */
        klog_cli.buffer[klog_cli.end] = k->buffer[i];
        klog_cli.end = (klog_cli.end + 1) % KLOG_BUFFER_SIZE;
        if (klog_cli.end == klog_cli.start) {
            klog_cli.start = (klog_cli.start + 1) % KLOG_BUFFER_SIZE;
        }

        if (kprintf) {
            #if CLI
            terminal_putc(TERM_MODE_TERM, k->buffer[i]);
            #endif
        } else {
            terminal_putc(TERM_MODE_TERM, k->buffer[i]);
        }

        klog_putchar_times++;
        i = (i + 1) % KLOG_BUFFER_SIZE;
    }

    klog_refresh(TERM_MODE_TERM);
    UNLOCK_LOCK(&klog_info_lock);
}

/**
 * @brief Virtual printf wrapper.
 *
 * @param level Printing level
 * @param s Format string
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

    KLOG out;
    out.start = 0;
    out.end = 0;
    out.term = NULL;

    if (level < KLOG_LVL_UNKNOWN) {
        if (timer_enabled && level != KLOG_LVL_NONE) {
            CPU *cpu = smp_get_curr_cpu(NO_FORCE_GET_CPU);
            uint64_t nanos = hpet_get_nanos();
            uint64_t nsecs = NANOS_TO_SECONDS(nanos);
            uint64_t nms = NANOS_TO_MILLIS(nanos) % 1000;
            uint64_t bootsecs = cmos_get_boot_time_seconds();

            STD_TIME t = {0};
            seconds_to_std_time(bootsecs + nsecs, &t);

            klog_vprintf_wrapper(&out, "%04d-%02d-%02d %02d:%02d:%02d %03d ",
                                1900 + t.year, t.month + 1, t.dom, t.hours, t.minutes,
                                t.seconds, nms);
            if (cpu) {
                klog_vprintf_wrapper(&out, "%02d", cpu->cpu_id);
            } else {
                klog_vprintf_wrapper(&out, "--");
            }

            PROCESS *pcurr = sched_get_curr_proc();
            if (pcurr) {
                klog_vprintf_wrapper(&out, "-%03d ", pcurr->id);
            } else {
                klog_vprintf_wrapper(&out, "---- ");
            }
        } else if (level != KLOG_LVL_NONE) {
            klog_vprintf_wrapper(&out, "0000-00-00 00:00:00 000 ------ ");
        }

        switch (level) {
            case KLOG_LVL_VERBOSE:
                klog_vprintf_wrapper(&out, "\e[34m[VERBOSE]\e[0m ");
                break;
            case KLOG_LVL_DEBUG:
                klog_vprintf_wrapper(&out, "\e[34m[DEBUG]\e[0m ");
                break;
            case KLOG_LVL_INFO:
                klog_vprintf_wrapper(&out, "\e[32m[INFO ]\e[0m ");
                break;
            case KLOG_LVL_WARN:
                klog_vprintf_wrapper(&out, "\e[33m[WARN ]\e[0m ");
                break;
            case KLOG_LVL_ERROR:
                klog_vprintf_wrapper(&out, "\e[31m[ERROR]\e[0m ");
                break;
            case KLOG_LVL_TAB:
                klog_vprintf_wrapper(&out, "\t");
                break;
            case KLOG_LVL_SRTUP:
                klog_vprintf_wrapper(&out, "\e[93m[SRTUP]\e[0m ");
                break;
            case KLOG_LVL_NONE:
            default:
                break;
        }

        va_list args;
        va_start(args, s);
        klog_vprintf_core(&out, s, args);
        va_end(args);

        klog_send(&out, 0);
    }
}

/**
 * @brief Main kernel printing function.
 *
 * @param s Format string
 * @param ... Variadic arguments
 */
void kprintf(const char *s, ...) {
    KLOG out;
    out.start = 0;
    out.end = 0;
    out.term = NULL;

    va_list args;
    va_start(args, s);
    klog_vprintf_core(&out, s, args);
    va_end(args);

    klog_send(&out, 1);
}