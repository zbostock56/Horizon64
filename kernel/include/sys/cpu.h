/**
 * @file cpu.h
 * @author Zack Bostock
 * @brief Information pertaining to CPU initialization and functions
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <globals.h>
#include <structs/control_registers_str.h>

/* GCC built-in cpuid headers */
#include <cpuid.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* Model specific register numbers */
/* Reference: https://wiki.osdev.org/CPU_Registers_x86-64 */
/* Another Reference: https://wiki.osdev.org/SYSENTER#AMD:_SYSCALL.2FSYSRET */
#define MSR_PAT                 (0x00000277)
#define MSR_FS_BASE_ADDR        (0xC0000100)
#define MSR_GS_BASE_ADDR        (0xC0000101)
#define MSR_KERN_GS_BASE_ADDR   (0xC0000102)
/* Also called the EFER */
#define MSR_EXTN_FEAT_ENABLE    (0xC0000080)
/* Ring 0 and Ring 3 Segment bases, as well as SYSCALL EIP. */
#define MSR_STAR                (0xC0000081)
/* The kernel's RIP SYSCALL entry for 64 bit software. */
#define MSR_LSTAR               (0xC0000082)
/* The kernel's RIP for SYSCALL in compatibility mode. */
#define MSR_CSTAR               (0xC0000083)
/* The low 32 bits are the SYSCALL flag mask. If a bit in this is set, the */
/* corresponding bit in rFLAGS is cleared.                                 */
#define MSR_SFMASK              (0xC0000084)

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/*
  For x86_64 CPU's, the Control Registers hold information regarding operation
  of the processsor.
*/

#define read_cr(reg) ({      \
    unsigned long val;       \
    __asm__ __volatile__ (   \
        "mov %%" #reg ", %0" \
        : "=r" (val)         \
    );                       \
    val;                     \
})

#define write_cr(reg, val) ({ \
    __asm__ __volatile__ (    \
        "mov %0, %%" #reg     \
        :                     \
        : "r" (val)           \
    );                        \
})

/* Use these macros with the enums from structs/control_registers_str.h */
#define READ_CR0_BIT(n) (n & read_cr(cr0))
#define READ_CR2_BIT(n) (n & read_cr(cr2))
#define READ_CR3_BIT(n) (n & read_cr(cr3))
#define READ_CR4_BIT(n) (n & read_cr(cr4))
#define READ_CR8_BIT(n) (n & read_cr(cr8))

#define WRITE_TO_CR0_BIT(n) (write_cr(cr0, read_cr(cr0) | (1 << n)))
#define WRITE_TO_CR2_BIT(n) (write_cr(cr2, read_cr(cr2) | (1 << n)))
#define WRITE_TO_CR3_BIT(n) (write_cr(cr3, read_cr(cr3) | (1 << n)))
#define WRITE_TO_CR4_BIT(n) (write_cr(cr4, read_cr(cr4) | (1 << n)))
#define WRITE_TO_CR8_BIT(n) (write_cr(cr5, read_cr(cr5) | (1 << n)))

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void cpu_init(size_t cpu_number);
