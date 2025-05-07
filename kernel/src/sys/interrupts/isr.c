/**
 * @file isr.c
 * @author Zack Bostock
 * @brief Interrupt Service Routine setup and helpers.
 * @verbatim
 * In this file, there are helpers for registering handlers for specific
 * Interrupt Service Routines (ISR). In addition, there is the generic ISR
 * handler. Plus, the initialization function for the ISRs.
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <common/kprint.h>

#include <sys/interrupts/isr.h>
#include <sys/asm.h>
#include <sys/panic.h>

#include <proc/ctxsw.h>

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

static ISR_HANDLER g_isr_handlers[X86_64_IDT_ENTRIES] = {0};
static int available_vectors = 0x81;

/* From the auto generated file */
void isr_init_entries();

/**
 * @brief Initialization of ISRs.
 */
void isr_init() {
    klogs("INIT ISR: starting...\n");
    isr_init_entries();

    /* Set all to open (will cause #GP if a gate is not open and is accessed) */
    for (int i = 0; i < 256; i++) {
        idt_enable_gate(i);
    }

    idt_disable_gate(0x80);
    klogs("INIT ISR: finished...\n");
}

/**
 * @brief Helper to get an available ISR vector
 * @note Starts at 0x81 (129)
 *
 * @return int Available ISR vector number
 */
int isr_get_avaiable_vector() {
    while (available_vectors < 256) {
        if (!g_isr_handlers[available_vectors]) {
            return available_vectors;
        }
        available_vectors++;
    }
    kloge("ISR: No more available vectors!\n");
    halt();
}

/**
 * @brief Helper to enable the IDT gate for system calls
 */
void isr_enable_system_calls() {
    klogd("ISR: Opening gate 0x80 (128) for system calls\n");
    idt_enable_gate(0x80);
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
 * NOTE: If ISR is from different ring, then SS will be set to 0x0
 *
 * @param regs Information about the calling process.
 */
void isr_handler(REGISTERS *regs) {
  /* Check for spurious interrupt or system call interrupt */
  if (regs->interrupt == 39 || regs->interrupt == 128) {
    klogd("ISR: Received spurious or system call interrupt\n");
    return;
  }

  if (regs->interrupt > 128) {
    klogd("ISR: received software interrupt for scheduling\n");
  }

  /* Process interrupt */
  if (g_isr_handlers[regs->interrupt] != NULL) {
    /* Call general vector to service interrupt */
    g_isr_handlers[regs->interrupt](regs);
    return;
  } else if (regs->interrupt >= 32) {
    /* Unreserved interrupt with no handler, hang the system */
    kloge("Unhandled interrupt %d!\n\n", regs->interrupt);
    backtrace(0x0);
    halt();
  } else {
    /* Reserved interrupt, hang the system */
    PROCESS *p = sched_get_curr_proc();
    if (p) {
        kloge("Unhandled Exception for process %d (%s)! %s with error code %x (%d).\n\n",
              p->id, p->name, exceptions[regs->interrupt], regs->error_code,
              regs->error_code);
    } else {
        kloge("Unhandled Exception! %s with error code %x (%d).\n\n",
              exceptions[regs->interrupt], regs->error_code, regs->error_code);
    }
    backtrace(regs->rip);
    uint64_t cr2 = read_cr(cr2);
    uint64_t cr3 = read_cr(cr3);
    uint64_t cr4 = read_cr(cr4);
    klogn("\nRIP   : (%x)\nCS    : (%x)\nRFLAGS: (%x)\n"
            "RSP   : (%x)\nSS    : (%x)\n"
            "RAX   : %x\nRBX   : %x\nRCX   : %x\nRDX   : %x\n"
            "RSI   : %x\nRDI   : %x\nRBP   : %x\n"
            "R8    : %x\nR9    : %x\nR10   : %x\nR11   : %x\n"
            "R12   : %x\nR13   : %x\nR14   : %x\nR15   : %x\n"
            "CR2   : %x\nCR3   : %x\nCR4   : %x\n\n",
            regs->rip, regs->cs, regs->rflags, regs->rsp, regs->ss, regs->rax,
            regs->rbx, regs->rcx, regs->rdx, regs->rsi, regs->rdi, regs->rbp,
            regs->r8, regs->r9, regs->r10, regs->r11, regs->r12, regs->r13,
            regs->r14, regs->r15, cr2, cr3, cr4);
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
