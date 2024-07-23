/**
 * @file memory.c
 * @author Zack Bostock
 * @brief Memory helpers, needed for compiler
 * @verbatim
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <common/memory.h>

/**
 * @brief Copies memory from one location to another.
 *
 * @param dst Dest pointer
 * @param src Source pointer
 * @param num Size of memory to copy
 * @return void* Destination where memory was copied to
 */
void *memcpy(void *dst, const void *src, size_t num) {
  uint8_t *u8Dst = (uint8_t *)dst;
  const uint8_t *u8Src = (const uint8_t *)src;
  for (size_t i = 0; i < num; i++) {
    u8Dst[i] = u8Src[i];
  }
  return dst;
}

/**
 * @brief
 *
 * @param ptr Memory location to start from
 * @param value Value to set it to
 * @param num Number of bytes to set
 * @return void* Pointer to the memory which was set
 */
void *memset(void *ptr, int value, size_t num) {
  uint8_t *u8Ptr = (uint8_t *)ptr;
  for (size_t i = 0; i < num; i++) {
    u8Ptr[i] = (uint8_t)value;
  }
  return ptr;
}

/**
 * @brief Compares to pieces of memory to another
 *
 * @param ptr1 First memory location to compare to
 * @param ptr2 Second memory location
 * @param num Number of bytes to compare
 * @return int 1 if different, 0 if not
 */
int memcmp(const void *ptr1, const void *ptr2, size_t num) {
  const uint8_t *u8Ptr1 = (const uint8_t *)ptr1;
  const uint8_t *u8Ptr2 = (const uint8_t *)ptr2;

  for (size_t i = 0; i < num; i++) {
    if (u8Ptr1[i] != u8Ptr2[i]) {
      return 1;
    }
  }
  return 0;
}

/**
 * @brief Moves memory from one place to another.
 *
 * @param dest Destination of where to move to
 * @param src Source of where the data is
 * @param n Number of bytes to move
 * @return void* Pointer to the destination
 */
void *memmove(void *dest, const void *src, size_t n) {
  uint8_t *pdest = (uint8_t *)dest;
  const uint8_t *psrc = (const uint8_t *)src;
  if (src > dest) {
    for (size_t i = 0; i < n; i++) {
      pdest[i] = psrc[i];
    }
  } else if (src < dest) {
    for (size_t i = n; i > 0; i--) {
      pdest[i - 1] = psrc[i - 1];
    }
  }
  return dest;
}
