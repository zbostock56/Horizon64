#include <kernel.h>

void _start() {

  /* Sets vital system settings */
  system_init();

  terminal_printf(&term, "Horizon OS\n");
  while (1) {}
}