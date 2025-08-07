/**
 * @file strchr.c
 * @author Zack Bostock
 * @brief Finds the first occurance of a character in a string
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <string.h>

/**
 * @brief Finds first occurrence of character in string
 *
 * @param s String to search
 * @param c Character to find
 * @return char* Pointer to found character, or NULL if not found
 */
char *strchr(const char *s, int c) {
    char ch = (char)c;
    while (*s) {
        if (*s == ch) {
            return (char *)s;
        }
        s++;
    }
    return (ch == '\0') ? (char *)s : NULL;
}