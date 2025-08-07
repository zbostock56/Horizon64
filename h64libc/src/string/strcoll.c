/**
 * @file strcoll.c
 * @author Zack Bostock
 * @brief Compares two strings according to locale.
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <string.h>

/**
 * @brief Compares two strings according to locale
 * @note Locales are not currently implemented
 *
 * @param s1 First string
 * @param s2 Second string
 * @return int Negative if s1 < s2, 0 if equal, positive if s1 > s2
 */
int strcoll(const char *s1, const char *s2) {
    return strcmp(s1, s2);
}