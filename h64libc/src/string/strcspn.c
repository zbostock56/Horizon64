/**
 * @file strcspn.c
 * @author Zack Bostock
 * @brief Calculates length of initial segment not containing
 *        reject characters
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <string.h>

/**
 * @brief Calculates length of initial segment not containing
 *        reject characters
 *
 * @param s1 String to examine
 * @param s2 String containing reject characters
 * @return size_t Length of initial segment
 */
size_t strcspn(const char *s1, const char *s2) {
    const char *p, *a;
    size_t count = 0;

    for (p = s1; *p; p++) {
        for (a = s2; *a; a++) {
            if (*p == *a) {
                return count;
            }
        }
        count++;
    }
    return count;
}