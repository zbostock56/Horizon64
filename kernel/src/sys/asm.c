/**
 * @file asm.c
 * @author Zack Bostock
 * @brief Commonly used assembly instructions wrapped in C functions.
 * @verbatim
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <sys/asm.h>
#include <globals.h>

/**
 * @brief 8-bit function which utilizes the out instruction.
 * 
 * @param port Port to output to
 * @param val Value output to port
 */
void outb(int16_t port, uint8_t val) {
__asm__ volatile ("outb %b0, %w1"
                  :
                  : "a"(val), "Nd"(port)
                  : "memory");
}

/**
 * @brief 8-bit function which utilizes the in instruction.
 * 
 * @param port Port to read from
 * @return uint8_t Returns 8-bit value read from the port
 */
uint8_t inb(uint16_t port) {
  uint8_t ret;
  __asm__ volatile ("inb %w1, %b0"
                    : "=a"(ret)
                    : "Nd"(port)
                    : "memory");
  return ret;
}

/**
 * @brief Used for waiting on specific operations when doing port
 *        manipulations.
 */
void io_wait() {
  outb(0x80, 0);
}

/**
 * @brief Halts the processor and disables interrupts.
 */
__attribute__((noreturn)) void halt() {
  disable_interrupts();
    __asm__ volatile("hlt");
    for(;;);
}

/**
 * @brief Disables external interrupts from being serviced by the processor.
 */
void disable_interrupts() {
  __asm__ volatile("cli");
}

/**
 * @brief Enables external interrupts to be serviced from the processor.
 */
void enable_interrupts() {
  __asm__ volatile("sti");
}