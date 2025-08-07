/**
 * @file memset.c
 * @author Zack Bostock
 * @brief Sets memory to a specific value
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <string.h>

/**
 * @brief Sets memory to a specific value
 *
 * @param s Pointer to memory area
 * @param c Value to set (converted to unsigned char)
 * @param n Number of bytes to set
 * @return void* Pointer to memory area
 */
void *memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    unsigned char uc = (unsigned char)c;

    /* Handle small sizes byte by byte */
    if (n < sizeof(size_t) * 4) {
        for (size_t i = 0; i < n; i++) {
            p[i] = uc;
        }
        return s;
    }

    /* Align to word boundary */
    while ((uintptr_t)p & (sizeof(size_t) - 1)) {
        if (n == 0) return s;
        *p++ = uc;
        n--;
    }

    /* Create word-sized pattern */
    size_t pattern = 0;
    for (size_t i = 0; i < sizeof(size_t); i++) {
        pattern |= ((size_t)uc) << (i * 8);
    }

    /* Set word-aligned chunks */
    size_t *wp = (size_t *)p;
    while (n >= sizeof(size_t) * 4) {
        wp[0] = pattern;
        wp[1] = pattern;
        wp[2] = pattern;
        wp[3] = pattern;
        wp += 4;
        n -= sizeof(size_t) * 4;
    }

    while (n >= sizeof(size_t)) {
        *wp++ = pattern;
        n -= sizeof(size_t);
    }

    /* Handle remaining bytes */
    p = (unsigned char *)wp;
    while (n > 0) {
        *p++ = uc;
        n--;
    }

    return s;
}