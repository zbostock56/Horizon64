/**
 * @file string.c
 * @author Zack Bostock
 * @brief Functionality pertaining to string manipulation
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <common/string.h>

/**
 * @brief string copying function for a specific number of bytes
 *
 * @param destination destination in memory to copy to
 * @param src source string to copy
 * @param num number of bytes to copy
 * @return char * returns a pointer to the destination string dest
 */
char *strncpy(char *dest, const char *src, size_t num) {
    if (!dest) {
        return NULL;
    }

    size_t i;

    for (i = 0; i < num && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < num; i++) {
        dest[i] = '\0';
    }
    return dest;
}

/**
 * @brief Finds the length of a string using the null terminator
 *
 * @param str String to find length of
 * @return size_t Length of string based on null terminator
 */
size_t strlen(const char *str) {
    const char *s = str;
    while (*s) {
        ++s;
    }
    return s - str;
}
