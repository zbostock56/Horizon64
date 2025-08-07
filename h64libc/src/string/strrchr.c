/**
 * @file strrchr.c
 * @author Zack Bostock
 * @brief Finds the last occurrence of character in string
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <string.h>

/**
 * @brief Finds last occurrence of character in string
 *
 * @param s String to search
 * @param c Character to find
 * @return char* Pointer to found character, or NULL if not found
 */
char *strrchr(const char *s, int c) {
    char ch = (char)c;
    const char *last = NULL;

    while (*s) {
        if (*s == ch) {
            last = s;
        }
        s++;
    }

    if (ch == '\0') {
        return (char *)s;
    }

    return (char *)last;
}