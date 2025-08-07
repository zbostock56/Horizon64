/**
 * @file memcpy.c
 * @author Zack Bostock
 * @brief Copies memory from one location to another
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <string.h>

/**
 * @brief Copies memory from one location to another
 *
 * @param dst Dest pointer
 * @param src Source pointer
 * @param num Size of memory to copy
 * @return void* Destination where memory was copied to
 */
void *memcpy(void *__restrict dest, const void *__restrict src, size_t n) {
    char *d = (char *)dest;
    const char *s = (const char *)src;
    
    /* Handle small copies byte by byte */
    if (n < sizeof(size_t) * 4) {
        for (size_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
        return dest;
    }
    
    /* Align destination to word boundary */
    while ((uintptr_t)d & (sizeof(size_t) - 1)) {
        if (n == 0) return dest;
        *d++ = *s++;
        n--;
    }
    
    /* Copy word-aligned chunks */
    size_t *wd = (size_t *)d;
    const size_t *ws = (const size_t *)s;
    
    /* Unroll loop for better performance */
    while (n >= sizeof(size_t) * 4) {
        wd[0] = ws[0];
        wd[1] = ws[1];
        wd[2] = ws[2];
        wd[3] = ws[3];
        wd += 4;
        ws += 4;
        n -= sizeof(size_t) * 4;
    }
    
    /* Handle remaining word-aligned bytes */
    while (n >= sizeof(size_t)) {
        *wd++ = *ws++;
        n -= sizeof(size_t);
    }
    
    /* Handle remaining bytes */
    d = (char *)wd;
    s = (const char *)ws;
    while (n > 0) {
        *d++ = *s++;
        n--;
    }
    
    return dest;
}