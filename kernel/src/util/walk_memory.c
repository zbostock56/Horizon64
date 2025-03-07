/**
 * @file walk_memory.c
 * @author Zack Bostock
 * @brief Helper for walking through a memory space.
 * @verbatim
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <util/walk_memory.h>
#include <common/kprint.h>

/**
 * @brief Helper for walking through a memory space
 *
 * @param rsp Stack pointer to start at
 * @param depth How far to walk
 */
void walk_memory(unsigned long rsp, uint8_t depth) {
  uint64_t *ptr = (uint64_t *) rsp;
  klogd("------------ Walking memory ------------\n");
  for (int i = depth; i >= (-1 * depth); i--) {
    if (i < 0) {
        klogd("offset -%3d bytes (%x): %x\n", (8 * i) * -1, ptr + i, *(ptr + i));
    } else {
        klogd("offset %3d bytes  (%x): %x\n", i * 8, ptr + i, *(ptr + i));
    }
  }
}


/**
 * @brief Helper for printing out entire sections of the stack
 *
 * @param start Starting address (higher)
 * @param end Ending Address (lower)
 */
void stack_print(unsigned long start, unsigned long end) {
    if (start < end) {
        unsigned long temp = start;
        start = end;
        end = temp;
    }

    uint64_t *ptr = (uint64_t *) start;
    klogd("--------- Printing stack from %x to %x -----------\n", start, end);
    for (unsigned int i = 0; i < ((start - end)) / 8; i++) {
        klogt("(%x): %x\n", ptr - i, *(ptr - i));
    }
    klogd("---------------------------------------------------\n");
}
