/**
 * @file strxfrm.c
 * @author Zack Bostock
 * @brief Transforms string according to locale for comparison
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <string.h>

/**
 * @brief Transforms string according to locale for comparison
 * @note Locales are not currently implemented
 *
 * @param dest Destination buffer for transformed string
 * @param src Source string to transform
 * @param n Size of destination buffer
 * @return size_t Length of transformed string
 */
size_t strxfrm(char *__restrict dest, const char *__restrict src, size_t n) {
    size_t len = strlen(src);
    if (dest && n > 0) {
        size_t copy_len = (len < n - 1) ? len : n - 1;
        memcpy(dest, src, copy_len);
        dest[copy_len] = '\0';
    }
    return len;
}