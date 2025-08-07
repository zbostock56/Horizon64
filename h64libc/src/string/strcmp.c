/**
 * @file strcmp.c
 * @author Zack Bostock
 * @brief Compare two strings
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <string.h>

/**
 * @brief Compares two strings
 *
 * @param s1 First string
 * @param s2 Second string
 * @return int Negative if s1 < s2, 0 if equal, positive if s1 > s2
 */
int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}