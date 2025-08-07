/**
 * @file strncmp.c
 * @author Zack Bostock
 * @brief Compares at most n character of two strings
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <string.h>

/**
 * @brief Compares at most n characters of two strings
 *
 * @param s1 First string
 * @param s2 Second string
 * @param n Maximum number of characters to compare
 * @return int Negative if s1 < s2, 0 if equal, positive if s1 > s2
 */
int strncmp(const char *s1, const char *s2, size_t n) {
    if (n == 0) return 0;
    
    while (n > 1 && *s1 && *s1 == *s2) {
        s1++;
        s2++;
        n--;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}