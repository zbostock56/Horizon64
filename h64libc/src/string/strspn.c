/**
 * @file strspn.c
 * @author Zack Bostock
 * @brief Calculates length of initial segment containing
 *        only accept characters
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <string.h>

/**
 * @brief Calculates length of initial segment containing only accept characters
 *
 * @param s1 String to examine
 * @param s2 String containing accept characters
 * @return size_t Length of initial segment
 */
size_t strspn(const char *s1, const char *s2) {
    const char *p, *a;
    size_t count = 0;

    for (p = s1; *p; p++) {
        for (a = s2; ; a++) {
            if (*a == '\0') {
                return count;
            }
            if (*p == *a) {
                break;
            }
        }
        count++;
    }
    return count;
}