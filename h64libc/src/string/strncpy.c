/**
 * @file strncpy.c
 * @author Zack Bostock
 * @brief Copies at most n characters from source to destination
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <string.h>

/**
 * @brief Copies at most n characters from source to destination
 *
 * @param dest Destination string buffer
 * @param src Source string
 * @param n Maximum number of characters to copy
 * @return char* Pointer to destination string
 */
char *strncpy(char *__restrict dest, const char *__restrict src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}