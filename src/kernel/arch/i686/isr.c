#include "isr.h"
#include "idt.h"
#include "gdt.h"
#include "general.h"
#include <stdio.h>
#include <stddef.h>

isr_handler g_isr_handlers[i686_IDT_ENTRIES];

static const char *const g_Exceptions[] = {
    "Divide by Zero Error",
    "Debug",
    "Non-maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception ",
    "",
    "",
    "",
    "",
    "",
    "",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    ""
};

void i686_isr_init_gates();

void i686_isr_init() {
    i686_isr_init_gates();
    for (int i = 0; i < 256; i++) {
        i686_idt_enable_gate(i);
    }
    i686_idt_disable_gate(0x80);
}

void __attribute__((cdecl)) i686_isr_handler(REGISTERS *regs) {
    if (g_isr_handlers[regs->interrupt] != NULL) {
        g_isr_handlers[regs->interrupt](regs);
    } else if (regs->interrupt >= 32) {
        printf("Unhandled interrupt %d!\n", regs->interrupt);
        i686_panic();
    } else {
        printf("\n\nKERNEL PANIC\n");
        printf("Unhandled exception %d (%s)\n", regs->interrupt,
                                              g_Exceptions[regs->interrupt]);
        printf("eax (0x%x)\nebx (0x%x)\necx (0x%x)\nedx (0x%x)\nesi (0x%x)\nedi (0x%x)\n",
                regs->eax, regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi);
        printf("esp (0x%x)\nebp (0x%x)\neip (0x%x)\neflags (0x%x)\ncs (0x%x)\nds (0x%x)\nss (0x%x)\n",
                regs->esp, regs->ebp, regs->eip, regs->eflags, regs->cs, regs->ds, regs->ss);
        i686_panic();
    }
}

void i686_isr_register_handler(int interrupt, isr_handler handler) {
    g_isr_handlers[interrupt] = handler;
    i686_idt_enable_gate(interrupt);
}
