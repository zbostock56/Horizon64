/**
 * @file control_registers_str.h
 * @author Zack Bostock
 * @brief Information regarding the cpu control registers and their bits'
 *        meanings
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

/*
  For x86_64 CPU's, the Control Registers hold information regarding operation
  of the processor.

  Reference: https://wiki.osdev.org/CPU_Registers_x86-64#Control_Registers

  Here, we define what each part of them means.
*/

/*
  Control Register 0:

  This control register contains various flags that modify the basic
  operation of the processor.

  Description of bits:
  - (0) PE: If set, the process is in protected mode,
        otherwise, it is in real mode.
  - (1) MP: Controls interaction of WAIT/FWAIT instructions
        with the TS flag in CR0.
  - (2) EM: If set, no x87 FPU is present, if clear, x87 FPU
        is present.
  - (3) TS: Allows saving x87 task context upon a task switch
        only after an x87 instruction is used.
  - (4) ET: On the i386 CPU's, it allowed specifying whether to the
        external math co-processor was an 80287 or 80387.
  - (5) NE: Enables internal x87 floating-point error reporting when
        set; otherwise, enables PC-style x87 error detection.
  - (16) WP: When set, the CPU cannot write to read-only pages when
         the privilege level is 0 (ring 0 mode).
  - (18) AM: Enables alignment checking if set, AC is set, and privilege
         level is 3 (ring 3 mode).
  - (29) NW: Globally enables/disables write-through caching.
  - (30) CD: Globally enables/disables the memory cache.
  - (31) PG: If set, enables paging and uses the CR3 register; otherwise,
         disables paging.
*/
typedef enum {
  CR0_PROCTECTED_MODE_ENABLED = 0,
  CR0_MONITOR_CO_PROCESSOR    = 1,
  CR0_EMULATION               = 2,
  CR0_TASK_SWITCHED           = 3,
  CR0_EXTENSION_TYPE          = 4,
  CR0_NUMERIC_ERROR           = 5,
  /* Bits 6 - 15 are reserved */
  CR0_WRITE_PROTECTED         = 16,
  /* Bit 17 is reserved */
  CR0_ALIGNMENT_MASK          = 18,
  /* Bits 19 - 28 are reserved */
  CR0_NOT_WRITE_THROUGH       = 29,
  CR0_CACHE_DISABLE           = 30,
  CR0_PAGING                  = 31,
  /* Bits 32 - 63 are reserved */
} CRO0_VALUES;

/*
  Control Register 2:

  This control register contains the linear (virtual) address which triggered
  a page fault, available for the page fault's interrupt handler.
*/

/*
  Control Register 3:

  This control register is used to control the paging structure hierarchy.
  It contains the physical address of the base of the page-structure
  hierarchy, which is used to translate virtual address to physical addresses.

  Description of the bits:
  If Process Context Identifier (PCID) is unset (PCID found in CR4):
  ---------------
  |   (0 - 2): Reserved
  |   (3) PWT: Page-Level Write Through Flag
  |   (4) PCD: Page-Level Cache Disable Flag
  |   (5 - 11): Reserved
  ---------------

  If PCID is set:
  ---------------
  |   (0 - 11): PCID
  ---------------

  Regardless of if PCID is set or not:
  (12 - 63): Physical address of the base of the paging-structure heirarchy.
*/

typedef enum {
  CR3_PAGE_LEVEL_WRITE_THROUGH = 3,
  CR3_PAGE_LEVEL_CACHE_DISABLE = 4,
} CR3_VALUES;

/*
  Control Register 4:

  This control register is used for more processor configuration.

  Description of the bits:
  - (0) VME: Virtual-8086 Mode Extensions
  - (1) PVI: Protected Mode Virtual Interrupts
  - (2) TSD: Time Stamp enabled only in ring 0
  - (3) DE : Debugging Extensions
  - (4) PSE: Page Size Extension
  - (5) PAE: Physical Address Extension
  - (6) MCE: Machine Check Exception
  - (7) PGE: Page Global Enable
  - (8) PCE: Performance Monitoring Counter Enable
  - (9) OSFXSR: OS support for fxsave and fxrstor instructions
  - (10) OSXMMEXCPT: OS support for unmasked SIMD floating point exceptions
  - (11) UMIP: User-Mode Instruction Prevention (SGDT, SIDT, SLDT, SMSW, and
         STR are disabled in user mode)
  - (12): Reserved
  - (13) VMXE: Virtual Machine Extensions Enable
  - (14) SMXE: Safer Mode Extensions Enable
  - (15): Reserved
  - (16) FSGSBASE: Enables the instructions RDFSBASE, RDGSBASE, WRFSBASE,
         and WRGSBASE
  - (17) PCIDE: PCID Enable
  - (18) OSXSAVE: XSAVE and Processor Extended States Enable
  - (19): Reserved
  - (20) SMEP: Supervisor Mode Executions Protection Enable
  - (21) SMAP: Supervisor Mode Access Protection Enable
  - (22) PKE: Enable protection keys for user-mode pages
  - (23) CET: Enable Control-flow enforcement Technology
  - (24) PKS: Enable protection keys for supervisor-mode pages
  - (25-63): Reserved
*/

typedef enum {
  CR4_VIRTUAL_8086_MODE_EXTENSIONS                 = 0,
  CR4_PROTECTED_MODE_VIRTUAL_INTERRUPTS            = 1,
  CR4_TIME_STAMP                                   = 2,
  CR4_DEBUGGING_EXTENSIONS                         = 3,
  CR4_PAGE_SIZE_EXTENSION                          = 4,
  CR4_PHYSICAL_ADDRESS_EXTENSION                   = 5,
  CR4_MACHINE_CHECK_EXCEPTION                      = 6,
  CR4_PAGE_GLOBAL_ENABLE                           = 7,
  CR4_PERFORMANCE_MONITORING_COUNTER_ENABLE        = 8,
  CR4_FXSAVE_FXRSTOR_INSTRUCTIONS                  = 9,
  CR4_UNMASKED_SIMD_FLOATING_POINT_EXCEPTIONS      = 10,
  CR4_USER_MODE_INSTRUCTION_PREVENTION             = 11,
  CR4_VIRTUAL_MACHINE_EXTENSION_ENABLE             = 13,
  CR4_SAFER_MODE_EXTENSIONS_ENABLE                 = 14,
  CR4_FSGSBASE                                     = 16,
  CR4_PCID_ENABLE                                  = 17,
  CR4_PROCESSOR_EXTENDED_STATES_ENABLE             = 18,
  CR4_SUPERVISOR_MODE_EXECUTIONS_PROTECTION_ENABLE = 20,
  CR4_SUPERVISOR_MODE_ACCESS_PROTECTION_ENABLE     = 21,
  CR4_PROTECTION_KEYS_USER_MODE                    = 22,
  CR4_CONTROL_FLOW_ENFORCEMENT_TECHNOLOGY_ENABLE   = 23,
  CR4_PROTECTOIN_KEYS_SUPERVISOR_MODE_PAGES_ENABLE = 24,
} CR4_VALUES;

/*
  Control Register 8:

  This control register is accessable in 64-bit mode using the REX prefix.
  CR8 is used to prioritize external interrupts and is referred to as the
  Task-Priority Register (TPR). System software can use the TPR register
  to temporarily block low-priority interrupts from interrupting a high-
  priority task. This is accomplished by loading the TPR with a value
  corresponding to the highest-priority interupt that is to be blocked.
  Loading TPR with 0 enables all external interrupts. Loading TPR with 0xF
  disables all external interrupts. TPR is cleared to 0 on reset.

  Description of the bits:
  (0 - 3): Priority
  (4 - 63): Reserved
*/

/*
  Control registers 1, 5-7, 9-15:

  These are reserved, the CPU will through a #UD exception when trying
  to access them.
*/
