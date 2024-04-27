#include <isr.h>

/* From the auto generated file */
void isr_init_entries();

void isr_init() {
    isr_init_entries();
    for (int i = 0; i < 256; i++) {
        idt_enable_gate(i);
    }
    idt_disable_gate(0x80);
    enable_interrupts();
    kprintf("ISR's are initialized, interrupts are enabled\n");
}

/*
    Default Interrupt Vector Assignment:
    0-31: Reserved by Intel
    8-15: IRQ 0-7 by the BIOS bootstrap
    70h-78h: IRQ 8-15 by the BIOS bootstrap
*/
void isr_handler(int interrupt, REGISTERS *regs, uint64_t error_code) {

    /* TODO: Check for spurious interrupt */

    /* TODO: Set aside an ISR for dispatching system calls */

    /* TODO: Set aside an ISR for scheduling */

    /* Process interrupt */
    if (g_isr_handlers[interrupt] != NULL) {
        g_isr_handlers[interrupt](regs);
    } else if (interrupt >= 32) {
        kprintf("Unhandled interupt %d!\n", interrupt);
        halt();
    } else {
        kprintf("\n\nUnhandled exception of %s. Error code: %d (%x)\n",
        exceptions[interrupt], error_code, error_code);
        kprintf("RIP   : %x\nCS    : %x\nRFLAGS: %x\n"
          "RSP   : %x\nSS    : %x\n"
          "RAX %x  RBX %x  RCX %x  RDX %x\n"
          "RSI %x  RDI %x  RBP %x\n"
          "R8  %x  R9  %x  R10 %x  R11 %x\n"
          "R12 %x  R13 %x  R14 %x  R15 %x\n\n",
          regs->rip, regs->cs, regs->rflags, regs->rsp, regs->ss,
          regs->rax, regs->rbx, regs->rcx, regs->rdx, regs->rsi, regs->rdi, regs->rbp,
          regs->r8, regs->r9, regs->r10, regs->r11, regs->r12, regs->r13, regs->r14,
          regs->r15);
        halt();
    }

}

/*
    Here, specific interrupt numbers are married to their
    interrupt handler (vector) for futher processing. When
    an interrupt occurs, the "common" handler will try to
    use a vector that was registered here to service the interrupt.
*/
void isr_register_handler(int interrupt, ISR_HANDLER handler) {
    g_isr_handlers[interrupt] = handler;
    idt_enable_gate(interrupt);
}