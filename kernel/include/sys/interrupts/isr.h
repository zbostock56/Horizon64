#pragma once

#include <globals.h>

/* ----------------------------- STATIC GLOBALS ----------------------------- */
static char* exceptions[] = {
    [0] = "Division by Zero",
    [1] = "Debug",
    [2] = "Non-Maskable Interrupt",
    [3] = "Breakpoint",
    [4] = "Overflow",
    [5] = "Bound Range Exceeded",
    [6] = "Invalid Opcode",
    [7] = "Device Not Available",
    [8] = "Double Fault",
    [9] = "Co-processor Segment Overrun",
    [10] = "Invalid TSS",
    [11] = "Segment Not Present",
    [12] = "Stack-Segment Fault",
    [13] = "General Protection Fault",
    [14] = "Page Fault",
    [15] = "",
    [16] = "x87 Floating-Point Exception",
    [17] = "Alignment Check",
    [18] = "Machine Check",
    [19] = "SIMD Floating-Point Exception",
    [20] = "Virtualization Exception",
    [21] = "Control Protection Exception",
    [22] = "",
    [23] = "",
    [24] = "",
    [25] = "",
    [26] = "",
    [27] = "",
    [28] = "Hypervisor Injection Exception",
    [29] = "VMM Communication Exception",
    [30] = "Security Exception",
    [31] = "",
    [32] = "Reserved",
    [33] = "Reserved",
    [34] = "Reserved",
    [35] = "Reserved",
    [36] = "Reserved",
    [37] = "Reserved",
    [38] = "Reserved",
    [39] = "Reserved",
    [40] = "Reserved",
    [41] = "Reserved",
    [42] = "Reserved",
    [43] = "Reserved",
    [44] = "Reserved"
};

/* --------------------------------- DEFINES -------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void isr_handler(REGISTERS *regs);
void isr_init();
void isr_register_handler(int interrupt, ISR_HANDLER handler);

/* --------------------------- EXTERNALLY DEFINED --------------------------- */
void idt_enable_gate(int interrupt);
void idt_disable_gate(int interrupt);
void walk_memory(void *rsp, uint8_t depth);
void stack_walk(void *rbp, uint8_t depth);