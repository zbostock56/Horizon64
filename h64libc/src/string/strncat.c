/**
 * @file strncat.c
 * @author Zack Bostock
 * @brief Concatenates at most n characters from source
 *        to destination
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <string.h>

/**
 * @brief Concatenates at most n characters from source to destination
 *
 * @param dest Destination string buffer
 * @param src Source string to append
 * @param n Maximum number of characters to append
 * @return char* Pointer to destination string
 */
char *strncat(char *__restrict dest, const char *__restrict src, size_t n) {
    char *d = dest;
    while (*d) d++; /* Find end of dest */

    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        d[i] = src[i];
    }
    d[i] = '\0';

    return dest;
}