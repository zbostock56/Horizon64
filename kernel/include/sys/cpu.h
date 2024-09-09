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
#include <structs/cpu_str.h>

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
/* Holds the base address of the local Advanced Programmable Interrupt */
/* Controller.                                                         */
#define MSR_APIC_BASE           (0x0000001B)

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
STATUS check_cpu_support(CPUID_FEATURE feature);

/**
 * @brief Uses the the cpuid function from gcc to determine the availability
 *        of CPU specific features.
 *
 * @param feature Specific feature to check
 * @note feature will be OR'd with 0x80000000 upon the call to __get_cpuid
 * @ref https://github.com/gcc-mirror/gcc/blob/master/gcc/config/i386/cpuid.h
 *
 * @param eax EAX register to pass into cpuid
 * @param ebx EBX register to pass into cpuid
 * @param ecx ECX register to pass into cpuid
 * @param edx EDX register to pass into cpuid
 */
static inline STATUS cpuid(uint32_t feature, uint32_t *eax, uint32_t *ebx,
                           uint32_t *ecx, uint32_t *edx) {
    unsigned int ret = __get_cpuid(feature, eax, ebx, ecx, edx);
    if (ret != 1) {
        klogi("CPUID failed with feature input %x!!\n", feature);
        return SYS_ERR;
    }
    return SYS_OK;
}

/**
 * @brief Reads the model specific register specified.
 *
 * @param msr Certain model specific register
 * @return uint64_t Current value in msr
 */
static inline uint64_t read_msr(uint32_t msr) {
    uint32_t low, high;

    __asm__ volatile("mov %[msr], %%ecx;"
                     "rdmsr;"
                     "mov %%eax, %[low];"
                     "mov %%edx, %[high];"
                     : [low] "=g"(low), [high] "=g"(high)
                     : [msr] "g"(msr)
                     : "eax", "ecx", "edx");

    return (((uint64_t) high << 32) | low);
}

/**
 * @brief Writes 64 bits to a model specific register.
 *
 * @param msr MSR to write to
 * @param val Value to write to MSR.
 */
static inline void write_msr(uint32_t msr, uint64_t val) {
    uint32_t low = val & UINT32_MAX;
    uint32_t high = (val >> 32) & UINT32_MAX;

    __asm__ volatile("mov %[msr], %%ecx;"
                     "mov %[low], %%eax;"
                     "mov %[high], %%edx;"
                     "wrmsr;"
                     :
                     : [msr] "g"(msr), [low] "g"(low), [high] "g"(high)
                     : "eax", "ecx", "edx");
}