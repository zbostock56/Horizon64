/**
 * @file strcpy.c
 * @author Zack Bostock
 * @brief Copies a string from source to destination
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <string.h>

/**
 * @brief Copies a string from source to destination
 *
 * @param dest Destination string buffer
 * @param src Source string
 * @return char* Pointer to destination string
 */
char *strcpy(char *__restrict dest, const char *__restrict src) {
    char *d = dest;
    while ((*d++ = *src++) != '\0');
    return dest;
}