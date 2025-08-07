/**
 * @file strlen.c
 * @author Zack Bostock
 * @brief Calculates the length of string
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <string.h>

/**
 * @brief Calculates length of string
 *
 * @param s String to measure
 * @return size_t Length of string (not including null terminator)
 */
size_t strlen(const char *s) {
    const char *p = s;
    while (*p) p++;
    return p - s;
}