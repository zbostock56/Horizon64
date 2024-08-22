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

  terminal_printf(&term,
  "                     _                     __    _  _   \n"
  "  /\\  /\\ ___   _ __ (_) ____ ___   _ __   / /_  | || |  \n"
  " / /_/ // _ \\ | '__|| ||_  // _ \\ | '_ \\ | '_ \\ | || |_ \n"
  "/ __  /| (_) || |   | | / /| (_) || | | || (_) ||__   _|\n"
  "\\/ /_/  \\___/ |_|   |_|/___|\\___/ |_| |_| \\___/    |_|  \n"
  "                                                        \n");
  while (1) {}
}
