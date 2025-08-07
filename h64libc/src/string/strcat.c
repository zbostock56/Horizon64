/**
 * @file strcat.c
 * @author Zack Bostock
 * @brief Concatenates source string to destination string
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <string.h>

/**
 * @brief Concatenates source string to destination string
 *
 * @param dest Destination string buffer
 * @param src Source string to append
 * @return char* Pointer to destination string
 */
char *strcat(char *__restrict dest, const char *__restrict src) {
    char *d = dest;
    while (*d) d++; /* Find end of dest */
    while ((*d++ = *src++) != '\0');
    return dest;
}