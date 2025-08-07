/**
 * @file strstr.c
 * @author Zack Bostock
 * @brief Finds first occurrence of substring in string
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <string.h>

/**
 * @brief Finds first occurrence of substring in string.
 *
 * @param haystack String to search in
 * @param needle Substring to find
 * @return char* Pointer to found substring, or NULL if not found
 */
char *strstr(const char *haystack, const char *needle) {
    if (*needle == '\0') {
        return (char *)haystack;
    }

    while (*haystack) {
        const char *h = haystack;
        const char *n = needle;

        while (*h && *n && *h == *n) {
            h++;
            n++;
        }

        if (*n == '\0') {
            return (char *)haystack;
        }

        haystack++;
    }
    return NULL;
}
