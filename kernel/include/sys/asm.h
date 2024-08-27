/**
 * @file asm.h
 * @author Zack Bostock
 * @brief Commonly used assembly instructions wrapped in C functions.
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>

/**
 * @brief C-wrapped function for the outb x86 assembly instruction
 * 
 * @param port Port to write to
 * @param value Value to write to the port 
 */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * @brief C-wrapped function for the inb x86 assembly instruction
 * 
 * @param port Port to read from
 * @return uint8_t Value read from port 
 */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * @brief C-wrapped function for the outw x86 assembly instruction
 * 
 * @param port Port to write to
 * @param value Value to write to the port 
 */
static inline void outw(uint16_t port, uint16_t value) {
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * @brief C-wrapped function for the inw x86 assembly instruction
 * 
 * @param port Port to read from
 * @return uint16_t Value read from port 
 */
static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * @brief C-wrapped function for the outd x86 assembly instruction
 * 
 * @param port Port to write to
 * @param value Value to write to the port 
 */
static inline void outd(uint16_t port, uint32_t value) {
    __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * @brief C-wrapped function for the ind x86 assembly instruction
 * 
 * @param port Port to read from
 * @return uint32_t Value read from port 
 */
static inline uint32_t ind(uint16_t port) {
    uint32_t ret;
    __asm__ volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * @brief Used for waiting on specific operations when doing port
 *        manipulations.
 */
static inline void io_wait() {
    outb(0x80, 0);
}

/**
 * @brief Disables external interrupts from being serviced by the processor.
 */
static inline void disable_interrupts() {
    __asm__ volatile("cli");
}

/**
 * @brief Enables external interrupts to be serviced from the processor.
 */
static inline void enable_interrupts() {
    __asm__ volatile("sti");
}

/**
 * @brief Halts the processor and disables interrupts.
 */
static inline __attribute__((noreturn)) void halt() {
    disable_interrupts();
    __asm__ volatile("hlt");
    for(;;);
}
