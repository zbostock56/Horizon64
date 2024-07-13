/**
 * @file stack_walk.c
 * @author Zack Bostock
 * @brief Helper for printing out the stack if exception occurs.
 * @verbatim
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <util/stack_walk.h>

/**
 * @brief Helper to print out the stack.
 * @verbatim
 * Prints out the stack using the base pointer of the stack frame, then
 * finds the return address of the next frame by looking "above" (higher memory)
 * in the stack. Since Limine bootloader does not necessarily zero out
 * the base pointer when it takes control, we specific depth in order
 * to not dereference invalid memory.
 * 
 * @param rbp Base pointer from the current stack frame
 * @param depth How deep to search through the stack
 */
void stack_walk(void *rbp, uint8_t depth) {
  uint64_t *ptr = (uint64_t *) rbp;
  kprintf("\n------------ Stack Trace ------------\n");
  for (int i = 0; i < depth; i++) {
    kprintf("Frame: %d (%x)\n", i, ptr);
    ptr = (uint64_t *) *(ptr + 1);
    if (*ptr < 0xffffffff80000000) {
      kprintf("Frame: %d (%x)\nHit end of valid range!\n", i + 1, ptr);
      return;
    }
  }
  kprintf("\n");
}