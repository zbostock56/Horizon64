/**
 * @file isr.c
 * @author Zack Bostock
 * @brief Interrupt Service Routine setup and helpers.
 * @verbatim
 * In this file, there are helpers for registering handlers for specific
 * Interrupt Service Routines (ISR). In addition, there is the generic ISR
 * handler. Plus, the initialization function for the ISRs.
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <sys/interrupts/isr.h>

/* From the auto generated file */
void isr_init_entries();

/**
 * @brief Initialization of ISRs.
 */
void isr_init() {
  isr_init_entries();
  /* Set all to open (will cause #GP if a gate is not open and is accessed) */
  for (int i = 0; i < 256; i++) {
    idt_enable_gate(i);
  }
  idt_disable_gate(0x80);
  enable_interrupts();
  klogi("ISR's are initialized, interrupts are enabled\n");
}

/**
 * @brief Generic ISR handler
 * @verbatim
 * Default Interrupt Vector Assignment:
 *  0-31: Reserved by Intel
 *  8-15: IRQ 0-7 by the BIOS bootstrap
 *  70h-78h: IRQ 8-15 by the BIOS bootstrap
 * 
 * IRQ's are remapped to start at 0x20 (interrupt 32)
 * 
 * @param regs Information about the calling process.
 */
void isr_handler(REGISTERS *regs) {

  /* TODO: Check for spurious interrupt */

  /* TODO: Set aside an ISR for dispatching system calls */

  /* TODO: Set aside an ISR for scheduling */

  /* Process interrupt */
  if (g_isr_handlers[regs->interrupt] != NULL) {
    /* Call general vector to service interrupt */
    g_isr_handlers[regs->interrupt](regs);
  } else if (regs->interrupt >= 32) {
    /* Unreserved interrupt with no handler, hang the system */
    kloge("Unhandled interupt %d!\n\n", regs->interrupt);
    walk_memory((void *) regs->rbp, 8);
    halt();
  } else {
    /* Reserved interrupt, hang the system */
    kloge("\n\nUnhandled exception!\n%s.\nError code: %d (%x)\n\n",
          exceptions[regs->interrupt], regs->error_code, regs->error_code);
    walk_memory((void *) regs->rbp, 8);
    // stack_walk((void *) regs->rbp, 4);
    kloge("\nRIP   : (%x)\nCS    : (%x)\nRFLAGS: (%x)\n"
            "RSP   : (%x)\nSS    : (%x)\n"
            "RAX   : %x\nRBX   : %x\nRCX   : %x\nRDX   : %x\n"
            "RSI   : %x\nRDI   : %x\nRBP   : %x\n"
            "R8    : %x\nR9    : %x\nR10   : %x\nR11   : %x\n"
            "R12   : %x\nR13   : %x\nR14   : %x\nR15   : %x\n\n",
            regs->rip, regs->cs, regs->rflags, regs->rsp, regs->ss, regs->rax,
            regs->rbx, regs->rcx, regs->rdx, regs->rsi, regs->rdi, regs->rbp,
            regs->r8, regs->r9, regs->r10, regs->r11, regs->r12, regs->r13,
            regs->r14, regs->r15);
    halt();
  }
}

/**
 * @brief Helper for registering handlers for specific ISRs.
 * @verbatim
 * Here, specific interrupt numbers are married to their
 * interrupt handler (vector) for futher processing. When
 * an interrupt occurs, the "common" handler will try to
 * use a vector that was registered here to service the interrupt.
 * 
 * @param interrupt Interrupt number to associate to handler
 * @param handler ISR handler itself
 */
void isr_register_handler(int interrupt, ISR_HANDLER handler) {
  /* Set the vector which will be the function execution upon interruption */
  g_isr_handlers[interrupt] = handler;
  /* Open the gate to allow the interrupt to be serviced */
  idt_enable_gate(interrupt);
}
