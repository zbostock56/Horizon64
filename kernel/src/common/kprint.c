/**
 * @file kprint.c
 * @author Zack Bostock
 * @brief Internal logging functionality
 * @verbatim
 *
 * @copyright Copyright (c) 2025
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

/* Log level configuration */
#ifdef ENABLE_KLOG_DEBUG
    #define MIN_LOG_LEVEL KLOG_LVL_VERBOSE
#else
    #define MIN_LOG_LEVEL KLOG_LVL_INFO
#endif

/* Log level colors and names */
static const struct {
    const char *name;
    const char *color;
} LOG_LEVEL_INFO[] = {
    [KLOG_LVL_VERBOSE] = {"VERBOSE", "\e[34m"},
    [KLOG_LVL_DEBUG]   = {"DEBUG  ", "\e[34m"},
    [KLOG_LVL_INFO]    = {"INFO   ", "\e[32m"},
    [KLOG_LVL_WARN]    = {"WARN   ", "\e[33m"},
    [KLOG_LVL_ERROR]   = {"ERROR  ", "\e[31m"},
    [KLOG_LVL_SRTUP]   = {"SRTUP  ", "\e[93m"},
    [KLOG_LVL_TAB]     = {"       ", ""},
    [KLOG_LVL_NONE]    = {"       ", ""}
};

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

static int print_prefix = TRUE;

extern int boot_time_set;

/**
 * @brief Initialize a kernel log buffer
 *
 * @param k Kernel log to initialize
 */
static inline void klog_init_buffer(KLOG *k) {
    memset(k->buffer, 0, KLOG_BUFFER_SIZE);
    k->start = 0;
    k->end = 0;
}

/**
 * @brief Main initialization function for kernel logging
 */
void klog_init(void) {
    LOCK_LOCK(&klog_info_lock);

    klog_init_buffer(&klog_info);
    klog_init_buffer(&klog_cli);

    UNLOCK_LOCK(&klog_info_lock);
}

/**
 * @brief Print debug stats about logging
 */
void klog_print_debug_stats(void) {
    klogd("Clear times: %llu\n", klog_clear_times);
    klogd("Refresh times: %llu\n", klog_refresh_times);
    klogd("Putc times: %llu\n", klog_putchar_times);
}

/**
 * @brief Helper to toggle printing prefix
 *
 * @param toggle TRUE to enable, FALSE to disable
 */
void klog_toggle_print_prefix(int toggle) {
    print_prefix = toggle;
}

/**
 * @brief Used externally to lock the klog lock
 */
void klog_lock(void) {
    LOCK_LOCK(&klog_info_lock);
}

/**
 * @brief Used externally to unlock the klog lock
 */
void klog_unlock(void) {
    UNLOCK_LOCK(&klog_info_lock);
}

/**
 * @brief Calculate number of characters in circular buffer
 *
 * @param k Kernel log buffer
 * @return Number of characters in buffer
 */
static inline unsigned long klog_buffer_count(const KLOG *k) {
    if (k->end >= k->start) {
        return k->end - k->start;
    }
    return (KLOG_BUFFER_SIZE - k->start) + k->end;
}

/**
 * @brief Check if circular buffer is empty
 *
 * @param k Kernel log buffer
 * @return TRUE if empty, FALSE otherwise
 */
static inline int klog_buffer_empty(const KLOG *k) {
    return k->start == k->end;
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
    if (!s) {
        kputs(k, "(null)", width);
        return;
    }

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
    if (!terminal_need_redraw()) {
        terminal_refresh(mode);
        klog_refresh_times++;
        return;
    }

    KLOG *k = (mode == TERM_MODE_INFO) ? &klog_info : &klog_cli;
    terminal_clear(mode);

    if (klog_buffer_empty(k)) {
        terminal_set_redraw(FALSE);
        terminal_refresh(mode);
        klog_refresh_times++;
        return;
    }

    /* Print buffer contents, handling wrap-around */
    unsigned long count = klog_buffer_count(k);
    unsigned long i = k->start;

    for (unsigned long printed = 0; printed < count; printed++) {
        terminal_putc(mode, k->buffer[i]);
        klog_putchar_times++;
        i = (i + 1) % KLOG_BUFFER_SIZE;
    }

    klog_clear_times++;
    terminal_set_redraw(FALSE);
    terminal_refresh(mode);
    klog_refresh_times++;
}

/**
 * @brief Get hex digit character for a 4-bit value
 *
 * @param digit 4-bit value (0-15)
 * @return Hex character ('0'-'9', 'A'-'F')
 */
static inline char get_hex_digit(uint8_t digit) {
    return (digit <= 9) ? (digit + '0') : (digit - 10 + 'A');
}

/**
 * @brief Find the most significant bit position
 *
 * @param num Number to analyze
 * @return Position of MSB (0-63), or 0 if num is 0
 */
static inline uint32_t find_msb_pos(uint64_t num) {
    if (num == 0) return 0;

    uint32_t pos = 0;
    if (num >= 0x100000000ULL) { pos += 32; num >>= 32; }
    if (num >= 0x10000) { pos += 16; num >>= 16; }
    if (num >= 0x100) { pos += 8; num >>= 8; }
    if (num >= 0x10) { pos += 4; num >>= 4; }
    if (num >= 0x4) { pos += 2; num >>= 2; }
    if (num >= 0x2) { pos += 1; }

    return pos;
}

/**
 * @brief Internal helper function to print numbers in hexadecimal
 *
 * @param k Kernel log to write to
 * @param num Number to print as hex
 * @param width Minimum number of hex digits to print
 */
static void kprint_hex(KLOG *k, uint64_t num, uint32_t width) {
    if (num == 0) {
        if (print_prefix) {
            kputs(k, "0x", 0);
        }
        /* Print at least one zero, or width zeros */
        int zeros_to_print = (width > 0) ? width : 1;
        for (int i = 0; i < zeros_to_print; i++) {
            kputc(k, '0');
        }
        return;
    }

    if (print_prefix) {
        kputs(k, "0x", 0);
    }

    /* Find the highest non-zero hex digit */
    int msb = find_msb_pos(num);
    int start_nibble = (msb / 4) * 4; /* Round down to nearest 4-bit boundary */

    /* Ensure we print at least 'width' digits */
    if (width > 0) {
        int min_start = (width - 1) * 4;
        if (start_nibble < min_start) {
            start_nibble = min_start;
        }
    }

    /* Print hex digits from most significant to least */
    for (int i = start_nibble; i >= 0; i -= 4) {
        uint8_t digit = (num >> i) & 0xF;
        kputc(k, get_hex_digit(digit));
    }
}

/**
 * @brief Helper for printing a number in binary
 *
 * @param k Kernel log to write to
 * @param num Number to print in binary
 * @param width Number of bits to print (0 for minimal representation)
 * @param spacing Whether to insert spaces between 4-bit groups
 */
static void kprint_bin(KLOG *k, uint64_t num, uint32_t width, uint8_t spacing) {
    if (num == 0) {
        if (print_prefix) {
            kputs(k, "0b", 0);
        }
        int bits_to_print = (width > 0) ? width : 1;
        for (int i = 0; i < bits_to_print; i++) {
            kputc(k, '0');
            if (spacing && (i % 4 == 3) && (i < bits_to_print - 1)) {
                kputc(k, ' ');
            }
        }
        return;
    }

    if (print_prefix) {
        kputs(k, "0b", 0);
    }

    int start_bit = (width > 0) ? (width - 1) : find_msb_pos(num);

    for (int i = start_bit; i >= 0; i--) {
        uint8_t bit = (num >> i) & 0x1;
        kputc(k, bit ? '1' : '0');

        if (spacing && (i % 4 == 0) && (i > 0)) {
            kputc(k, ' ');
        }
    }
}

/**
 * @brief Count digits in a positive integer
 *
 * @param num Positive integer
 * @return Number of digits
 */
static int count_digits(uint64_t num) {
    if (num == 0) return 1;

    int digits = 0;
    while (num > 0) {
        digits++;
        num /= 10;
    }
    return digits;
}

/**
 * @brief Helper method to print out an integer
 *
 * @param k Kernel log to write to
 * @param num Number to print
 * @param width Minimum width of output
 * @param zero_padding TRUE to pad with zeros, FALSE to pad with spaces
 */
static void kprint_int(KLOG *k, int64_t num, uint32_t width, uint8_t zero_padding) {
    if (num == 0) {
        /* Handle padding for zero */
        if (width > 1) {
            char pad_char = zero_padding ? '0' : ' ';
            for (uint32_t i = 0; i < width - 1; i++) {
                kputc(k, pad_char);
            }
        }
        kputc(k, '0');
        return;
    }

    int is_negative = (num < 0);
    uint64_t abs_num = is_negative ? -num : num;
    int digit_count = count_digits(abs_num);
    uint32_t total_chars = digit_count + (is_negative ? 1 : 0);

    /* Handle padding */
    if (width > total_chars) {
        if (is_negative && zero_padding) {
            kputc(k, '-');
            for (uint32_t i = 0; i < width - total_chars; i++) {
                kputc(k, '0');
            }
        } else {
            char pad_char = zero_padding ? '0' : ' ';
            for (uint32_t i = 0; i < width - total_chars; i++) {
                kputc(k, pad_char);
            }
            if (is_negative) {
                kputc(k, '-');
            }
        }
    } else if (is_negative) {
        kputc(k, '-');
    }

    /* Convert number to string in reverse order */
    char digits[32]; /* Enough for 64-bit number */
    int idx = 0;

    while (abs_num > 0) {
        digits[idx++] = (abs_num % 10) + '0';
        abs_num /= 10;
    }

    /* Print digits in correct order */
    for (int i = idx - 1; i >= 0; i--) {
        kputc(k, digits[i]);
    }
}

/**
 * @brief Format and print log level prefix
 *
 * @param k Kernel log to write to
 * @param level Log level
 */
static void kprint_level_prefix(KLOG *k, uint8_t level) {
    if (level >= sizeof(LOG_LEVEL_INFO) / sizeof(LOG_LEVEL_INFO[0])) {
        return;
    }

    if (level == KLOG_LVL_TAB) {
        kputc(k, '\t');
        return;
    }

    if (level == KLOG_LVL_NONE) {
        return;
    }

    /* Print colored level name */
    kputs(k, LOG_LEVEL_INFO[level].color, 0);
    kputc(k, '[');
    kputs(k, LOG_LEVEL_INFO[level].name, 0);
    kputs(k, "]\e[0m ", 0);
}

/**
 * @brief Core function for formatted printing using variadic arguments
 *
 * @param k Kernel log to write to
 * @param s Format string
 * @param args Variadic argument list
 */
static void klog_vprintf_core(KLOG *k, const char *s, va_list args) {
    if (!s) return;

    for (size_t i = 0; s[i] != '\0'; i++) {
        if (s[i] != '%') {
            kputc(k, s[i]);
            continue;
        }

        /* Parse format specifier */
        i++; /* Skip '%' */
        if (s[i] == '\0') break;

        if (s[i] == '%') {
            kputc(k, '%');
            continue;
        }

        uint32_t width = 0;
        uint8_t zero_padding = 0;

        /* Check for zero padding */
        if (s[i] == '0') {
            zero_padding = 1;
            i++;
        }

        /* Parse width */
        while (s[i] >= '0' && s[i] <= '9') {
            width = width * 10 + (s[i] - '0');
            i++;
        }

        /* Handle format specifiers */
        switch (s[i]) {
            case 'd':
                kprint_int(k, va_arg(args, int64_t), width, zero_padding);
                break;
            case 'x':
                kprint_hex(k, va_arg(args, uint64_t), width);
                break;
            case 'b':
                kprint_bin(k, va_arg(args, uint64_t), width, !zero_padding);
                break;
            case 's':
                kputs(k, va_arg(args, const char *), width);
                break;
            case 'c':
                kputc(k, (uint8_t)va_arg(args, int));
                break;
            case 't':
                kputs(k, va_arg(args, int) ? "true" : "false", 0);
                break;
            default:
                /* Unsupported format; print it literally */
                kputc(k, '%');
                kputc(k, s[i]);
                break;
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
 * @brief Format and print timestamp prefix
 *
 * @param k Kernel log to write to
 */
static void kprint_timestamp(KLOG *k) {
    if (!boot_time_set) {
        kputs(k, "0000-00-00 00:00:00 000 ------ ", 0);
        return;
    }

    CPU *cpu = smp_get_curr_cpu(NO_FORCE_GET_CPU);
    uint64_t nanos = hpet_get_nanos();
    uint64_t nsecs = NANOS_TO_SECONDS(nanos);
    uint64_t nms = NANOS_TO_MILLIS(nanos) % 1000;
    uint64_t bootsecs = cmos_get_boot_time_seconds();

    STD_TIME t = {0};
    seconds_to_std_time(bootsecs + nsecs, &t);

    /* Format: YYYY-MM-DD HH:MM:SS mmm CC-PPP */
    klog_vprintf_wrapper(k, "%04d-%02d-%02d %02d:%02d:%02d %03d ",
                        1900 + t.year, t.month + 1, t.dom,
                        t.hours, t.minutes, t.seconds, nms);

    if (cpu) {
        klog_vprintf_wrapper(k, "%02d", cpu->cpu_id);
    } else {
        kputs(k, "--", 0);
    }

    PROCESS *pcurr = sched_get_curr_proc();
    if (pcurr) {
        klog_vprintf_wrapper(k, "-%03d ", pcurr->id);
    } else {
        kputs(k, "---- ", 0);
    }
}

/**
 * @brief Sends the klog buffered input to its output destination
 *
 * @param k Kernel log to read from
 * @param use_kprintf Determines formatting behavior for terminal output
 */
static void klog_send(KLOG *k, uint8_t use_kprintf) {
    LOCK_LOCK(&klog_info_lock);

    unsigned long count = klog_buffer_count(k);
    if (count == 0) {
        UNLOCK_LOCK(&klog_info_lock);
        return;
    }

    /* Process and output characters from the log */
    unsigned long i = k->start;
    for (unsigned long printed = 0; printed < count; printed++) {
        uint8_t ch = k->buffer[i];

        /* Copy to CLI buffer */
        kputc(&klog_cli, ch);

        /* Output to terminal */
        if (use_kprintf) {
            #ifdef CLI
            terminal_putc(TERM_MODE_TERM, ch);
            #endif
        } else {
            terminal_putc(TERM_MODE_TERM, ch);
        }

        klog_putchar_times++;
        i = (i + 1) % KLOG_BUFFER_SIZE;
    }

    klog_refresh(TERM_MODE_TERM);
    UNLOCK_LOCK(&klog_info_lock);
}

/**
 * @brief Emergency print function that bypasses locking
 *
 * This function should ONLY be used in critical situations where the
 * normal logging system may be compromised (e.g., debugging locking code).
 * It writes directly to the serial output without any buffering or locking.
 *
 * @param s Format string
 * @param ... Variadic arguments
 */
void klog_emergency_printf(const char *s, ...) {
    if (!s) return;

    /* Create a temporary buffer for emergency output */
    static char emergency_buffer[512];
    char *buf = emergency_buffer;
    size_t buf_pos = 0;
    const size_t buf_size = sizeof(emergency_buffer) - 1;

    va_list args;
    va_start(args, s);

    /* Simple format string processing without full printf features */
    for (size_t i = 0; s[i] != '\0' && buf_pos < buf_size; i++) {
        if (s[i] != '%') {
            buf[buf_pos++] = s[i];
            continue;
        }

        i++; /* Skip '%' */
        if (s[i] == '\0') break;

        if (s[i] == '%') {
            buf[buf_pos++] = '%';
            continue;
        }

        /* Simple format handling - no width/precision support for emergency mode */
        switch (s[i]) {
            case 'd': {
                int64_t val = va_arg(args, int64_t);
                if (val == 0) {
                    if (buf_pos < buf_size) buf[buf_pos++] = '0';
                } else {
                    char digits[32];
                    unsigned int digit_count = 0;
                    int is_negative = (val < 0);

                    if (is_negative) {
                        val = -val;
                        if (buf_pos < buf_size) buf[buf_pos++] = '-';
                    }

                    while (val > 0 && digit_count < sizeof(digits)) {
                        digits[digit_count++] = (val % 10) + '0';
                        val /= 10;
                    }

                    for (int j = digit_count - 1; j >= 0 && buf_pos < buf_size; j--) {
                        buf[buf_pos++] = digits[j];
                    }
                }
                break;
            }
            case 'x': {
                uint64_t val = va_arg(args, uint64_t);
                if (val == 0) {
                    if (buf_pos < buf_size - 1) {
                        buf[buf_pos++] = '0';
                        buf[buf_pos++] = 'x';
                        buf[buf_pos++] = '0';
                    }
                } else {
                    if (buf_pos < buf_size - 1) {
                        buf[buf_pos++] = '0';
                        buf[buf_pos++] = 'x';
                    }

                    char hex_digits[16];
                    unsigned int hex_count = 0;

                    while (val > 0 && hex_count < sizeof(hex_digits)) {
                        uint8_t digit = val & 0xF;
                        hex_digits[hex_count++] = get_hex_digit(digit);
                        val >>= 4;
                    }

                    for (int j = hex_count - 1; j >= 0 && buf_pos < buf_size; j--) {
                        buf[buf_pos++] = hex_digits[j];
                    }
                }
                break;
            }
            case 's': {
                const char *str = va_arg(args, const char *);
                if (!str) str = "(null)";

                while (*str && buf_pos < buf_size) {
                    buf[buf_pos++] = *str++;
                }
                break;
            }
            case 'c': {
                if (buf_pos < buf_size) {
                    buf[buf_pos++] = (char)va_arg(args, int);
                }
                break;
            }
            default:
                /* Unknown format, just print it literally */
                if (buf_pos < buf_size - 1) {
                    buf[buf_pos++] = '%';
                    buf[buf_pos++] = s[i];
                }
                break;
        }
    }

    va_end(args);

    /* Null terminate the buffer */
    buf[buf_pos] = '\0';

    /* Output directly to serial without any locking or buffering */
    for (size_t i = 0; i < buf_pos; i++) {
        terminal_print(TERM_MODE_TERM, buf[i]);
    }
}

/**
 * @brief Virtual printf wrapper with log levels
 *
 * @param level Logging level
 * @param s Format string
 * @param ... Variadic arguments
 */
void klog_vprintf(uint8_t level, const char *s, ...) {
    /* Filter out messages based on log level */
    if (!ENABLE_KLOG_DEBUG && level < KLOG_LVL_INFO) {
        return;
    } else if (level >= KLOG_LVL_UNKNOWN) {
        return;
    }

    /* Create temporary output buffer */
    KLOG out;
    out.start = 0;
    out.end = 0;
    out.term = NULL;

    /* Add timestamp and log level prefixes */
    if (level != KLOG_LVL_NONE) {
        kprint_timestamp(&out);
        kprint_level_prefix(&out, level);
    }

    /* Format the actual message */
    va_list args;
    va_start(args, s);
    klog_vprintf_core(&out, s, args);
    va_end(args);

    /* Send to output */
    klog_send(&out, 0);
}

/**
 * @brief Main kernel printing function without log level formatting
 *
 * @param s Format string
 * @param ... Variadic arguments
 */
void kprintf(const char *s, ...) {
    KLOG out = {0};
    out.start = 0;
    out.end = 0;
    out.term = NULL;

    va_list args;
    va_start(args, s);
    klog_vprintf_core(&out, s, args);
    va_end(args);

    klog_send(&out, 1);
}