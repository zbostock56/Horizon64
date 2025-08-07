/**
 * @file strpbrk.c
 * @author Zack Bostock
 * @brief Finds first occurrence of any character from accept set
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <string.h>

/**
 * @brief Finds first occurrence of any character from accept set
 *
 * @param s1 String to search
 * @param s2 String containing characters to find
 * @return char* Pointer to found character, or NULL if none found
 */
char *strpbrk(const char *s1, const char *s2) {
    const char *p;

    while (*s1) {
        for (p = s2; *p; p++) {
            if (*s1 == *p) {
                return (char *)s1;
            }
        }
        s1++;
    }
    return NULL;
}