/**
 * @file memory.c
 * @author Zack Bostock
 * @brief Memory helpers, needed for compiler
 * @verbatim
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <stdint.h>
#include <common/memory.h>

/**
 * @brief Copies memory from one location to another.
 *
 * This version attempts to copy 8 bytes at a time when both the source
 * and destination pointers are aligned to sizeof(size_t). Any remaining
 * bytes are copied one at a time.
 *
 * @param dst Dest pointer
 * @param src Source pointer
 * @param num Size of memory to copy
 * @return void* Destination where memory was copied to
 */
void *memcpy(void *dst, const void *src, size_t num) {
    uint8_t *d8 = (uint8_t *)dst;
    const uint8_t *s8 = (const uint8_t *)src;

    /* Try to align pointers to size_t boundaries for faster copying */
    if ((((uintptr_t)d8 | (uintptr_t)s8) % sizeof(size_t)) == 0) {
        size_t *d64 = (size_t *)d8;
        const size_t *s64 = (const size_t *)s8;
        size_t num64 = num / sizeof(size_t);
        size_t remainder = num % sizeof(size_t);

        /* Unroll the loop for blocks of four size_t words if possible */
        while (num64 >= 4) {
            d64[0] = s64[0];
            d64[1] = s64[1];
            d64[2] = s64[2];
            d64[3] = s64[3];
            d64 += 4;
            s64 += 4;
            num64 -= 4;
        }
        while (num64--) {
            *d64++ = *s64++;
        }
        d8 = (uint8_t *)d64;
        s8 = (const uint8_t *)s64;
        num = remainder;
    }

    /* Copy remaining */
    for (size_t i = 0; i < num; i++) {
        d8[i] = s8[i];
    }

    return dst;
}

/**
 * @brief Sets memory to a given value.
 *
 * This version sets memory in 8-byte chunks when possible by preparing a
 * word with all bytes set to the given value. Any remaining bytes are set one
 * at a time.
 *
 * @param ptr Memory location to start from
 * @param value Value to set it to
 * @param num Number of bytes to set
 * @return void* Pointer to the memory which was set
 */
void *memset(void *ptr, int value, size_t num) {
    uint8_t *p8 = (uint8_t *)ptr;

    /* Prepare a size_t word filled with the byte value */
    size_t val_word = (unsigned char)value;
    val_word |= val_word << 8;
    val_word |= val_word << 16;
#if UINTPTR_MAX > 0xFFFFFFFF
    /* 64-bit: extend to 64 bits */
    val_word |= val_word << 32;
#endif

    if (((uintptr_t)p8 % sizeof(size_t)) == 0) {
        size_t *p64 = (size_t *)p8;
        size_t num64 = num / sizeof(size_t);
        size_t remainder = num % sizeof(size_t);

        /* Unroll loop for four words at a time */
        while (num64 >= 4) {
            p64[0] = val_word;
            p64[1] = val_word;
            p64[2] = val_word;
            p64[3] = val_word;
            p64 += 4;
            num64 -= 4;
        }
        while (num64--) {
            *p64++ = val_word;
        }
        p8 = (uint8_t *)p64;
        num = remainder;
    }

    for (size_t i = 0; i < num; i++) {
        p8[i] = (unsigned char)value;
    }

    return ptr;
}

/**
 * @brief Compares two pieces of memory.
 *
 * @param ptr1 First memory location to compare to
 * @param ptr2 Second memory location
 * @param num Number of bytes to compare
 * @return int 1 if different, 0 if not
 */
int memcmp(const void *ptr1, const void *ptr2, size_t num) {
    const uint8_t *p1 = (const uint8_t *)ptr1;
    const uint8_t *p2 = (const uint8_t *)ptr2;
    for (size_t i = 0; i < num; i++) {
        if (p1[i] != p2[i]) {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Moves memory from one place to another.
 *
 * This function safely handles overlapping memory areas. It uses the same
 * 8-byte copying strategy as memcpy when possible.
 *
 * @param dest Destination of where to move to
 * @param src Source of where the data is
 * @param n Number of bytes to move
 * @return void* Pointer to the destination
 */
void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *d8 = (uint8_t *)dest;
    const uint8_t *s8 = (const uint8_t *)src;

    if (s8 < d8 && d8 < s8 + n) {
        /* Overlap: copy backwards */
        d8 += n;
        s8 += n;
        while (n >= sizeof(size_t) && ((uintptr_t)d8 % sizeof(size_t)) == 0 &&
               ((uintptr_t)s8 % sizeof(size_t)) == 0) {
            d8 -= sizeof(size_t);
            s8 -= sizeof(size_t);
            *(size_t *)d8 = *(const size_t *)s8;
            n -= sizeof(size_t);
        }
        while (n--) {
            *(--d8) = *(--s8);
        }
    } else {
        /* Non-overlap: copy forward using memcpy implementation */
        return memcpy(dest, src, n);
    }
    return dest;
}
