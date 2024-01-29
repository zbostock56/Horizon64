#pragma once

#include <stdint.h>

typedef struct {
    /* Pushed in assembly */
    uint32_t ds;
    
    /* pusha instruction */
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t _unuseful;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    /* Error pushed automatically */
    uint32_t interrupt;
    uint32_t error;

    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp;
    uint32_t ss;
} __attribute__((packed)) REGISTERS;

typedef void (*isr_handler)(REGISTERS *);

void i686_isr_init();
void i686_isr_register_handler(int, isr_handler);