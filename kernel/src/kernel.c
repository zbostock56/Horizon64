/**
 * @file kernel.c
 * @author Zack Bostock
 * @brief Main entry point for kernel
 * @verbatim
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <kernel.h>

/**
 * @brief Main entry point from bootloader to kernel.
 */
void _start() {

  /* Sets vital system settings */
  system_init();

  terminal_printf(&term, "Horizon OS\n");
  while (1) {}
}
