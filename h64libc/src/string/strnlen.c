/**
 * @file strnlen.c
 * @author Zack Bostock
 * @brief Finds the length of a string using the null terminator, bounded by n
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <string.h>

/**
 * @brief Finds the length of a string using the null terminator, bounded by n
 *
 * @param s String to find length of
 * @param n Max length to stop at
 * @return size_t Length of string based on null terminator, or limit
 */
size_t strnlen(const char *s, size_t n) {
    const char *str = s;
    for (size_t i = 0; i < n; i++) {
        if (!(*str)) {
            return str - s;
        }
        str++;
    }
    return n;
}