/**
 * @file memchr.c
 * @author Zack Bostock
 * @brief Searches for a byte in memory
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <string.h>

/**
 * @brief Searches for a byte in memory
 *
 * @param s Memory area to search
 * @param c Byte to search for (converted to unsigned char)
 * @param n Number of bytes to search
 * @return void* Pointer to found byte, or NULL if not found
 */
void *memchr(const void *s, int c, size_t n) {
    const unsigned char *p = (const unsigned char *)s;
    unsigned char uc = (unsigned char)c;
    
    for (size_t i = 0; i < n; i++) {
        if (p[i] == uc) {
            return (void *)(p + i);
        }
    }
    return NULL;
}