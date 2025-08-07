/**
 * @file strtol.c
 * @author Zack Bostock
 * @brief Convert string to long integer
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <limits.h>

/**
 * @brief Converts string to long integer
 *
 * @param nptr String to convert
 * @param endptr Pointer to store address of first invalid character
 * @param base Number base (2-36, or 0 for auto-detection)
 * @return long Converted value, or 0 if no conversion possible
 */
long strtol(const char *nptr, char **endptr, register int base) {
    const char *s = nptr;
    long result = 0;
    int negative = 0;
    int overflow = 0;

    /* Skip leading whitespace */
    while (*s == ' ' || *s == '\t' || *s == '\n' ||
           *s == '\r' || *s == '\f' || *s == '\v') {
        s++;
    }

    /* Handle optional sign */
    if (*s == '-') {
        negative = 1;
        s++;
    } else if (*s == '+') {
        s++;
    }

    /* Auto-detect base if base is 0 */
    if (base == 0) {
        if (*s == '0') {
            if (s[1] == 'x' || s[1] == 'X') {
                base = 16;
                s += 2;
            } else {
                base = 8;
                s++;
            }
        } else {
            base = 10;
        }
    } else if (base == 16 && *s == '0' && (s[1] == 'x' || s[1] == 'X')) {
        /* Skip 0x prefix for explicit base 16 */
        s += 2;
    }

    /* Validate base */
    if (base < 2 || base > 36) {
        if (endptr) *endptr = (char *)nptr;
        return 0;
    }

    /* Calculate limits for overflow detection */
    long limit = negative ? -(unsigned long)LONG_MIN / base : LONG_MAX / base;
    int limit_digit = negative ? -(unsigned long)LONG_MIN % base : LONG_MAX % base;

    /* Convert digits */
    const char *start = s;
    while (*s) {
        int digit;

        if (*s >= '0' && *s <= '9') {
            digit = *s - '0';
        } else if (*s >= 'a' && *s <= 'z') {
            digit = *s - 'a' + 10;
        } else if (*s >= 'A' && *s <= 'Z') {
            digit = *s - 'A' + 10;
        } else {
            break;
        }

        if (digit >= base) {
            break;
        }

        /* Check for overflow */
        if (!overflow) {
            if (result > limit || (result == limit && digit > limit_digit)) {
                overflow = 1;
                result = negative ? LONG_MIN : LONG_MAX;
            } else {
                result = result * base + digit;
            }
        }

        s++;
    }

    /* Set endptr */
    if (endptr) {
        *endptr = (char *)(s == start ? nptr : s);
    }

    /* Apply sign and return */
    return negative ? -result : result;
}