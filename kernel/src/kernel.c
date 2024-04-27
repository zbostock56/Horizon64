#include <kernel.h>

void _start() {

  /* Sets vital system settings */
  system_init();

  terminal_puts(&term, "Horizon OS\n");

  /* TODO: Calculate available memory */

  halt();
}