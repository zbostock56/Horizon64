/**
 * @file memmove.c
 * @author Zack Bostock
 * @brief Moves memory from one location to another,
 *        handling overlapping memory regions
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <string.h>

/**
 * @brief Moves memory from one location to another, handling overlapping regions
 *
 * @param dest Destination pointer
 * @param src Source pointer
 * @param n Number of bytes to move
 * @return void* Pointer to destination
 */
void *memmove(void *dest, const void *src, size_t n) {
    char *d = (char *)dest;
    const char *s = (const char *)src;

    if (d == s || n == 0) {
        return dest;
    }

    if (d < s) {
        /* Copy forward */
        return memcpy(dest, src, n);
    } else {
        /* Copy backward to handle overlap */
        d += n - 1;
        s += n - 1;
        while (n > 0) {
            *d-- = *s--;
            n--;
        }
    }

    return dest;
}