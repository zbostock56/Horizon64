#include <util/walk_memory.h>

void walk_memory(void *rsp, uint8_t depth) {
  uint64_t *ptr = (uint64_t *) rsp;
  kprintf("------------ Walking memory ------------\n");
  for (int i = depth; i >= (-1 * depth); i--) {
    if (i < 0) {
      kprintf("offset -%d bytes (%x): %x\n", (8 * i) * -1, ptr + i, *(ptr + i));
    } else {
      kprintf("offset %d bytes  (%x): %x\n", i * 8, ptr + i, *(ptr + i));
    }
  }
}