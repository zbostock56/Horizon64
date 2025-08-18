/**
 * @file smp.h
 * @author Zack Bostock
 * @brief Information pertaining to Symmetric Multiprocessing (SMP)
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <globals.h>
#include <structs/smp_str.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
#define SMP_TRAMPOLINE_BLOCK_ADDR           (0x1000)
#define SMP_AP_BOOT_COUNTER_ADDR            (0xFF0)

#define SMP_TRAMPOLINE_ARG_IDTPTR           (0xFA0)
#define SMP_TRAMPOLINE_ARG_RSP              (0xFB0)
#define SMP_TRAMPOLINE_ARG_ENTRYPOINT       (0xFC0)
#define SMP_TRAMPOLINE_ARG_CR3              (0xFD0)
#define SMP_TRAMPOLINE_ARG_CPUINFO          (0xFE0)

/**
 * @brief These constants are used to pass into smp_get_curr_cpu to ensure that
 * even if the BSP is the only processor running, it will still get its info
 */
#define FORCE_GET_CPU                       (1)
#define NO_FORCE_GET_CPU                    (0)

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
SMP_INFO *smp_get_info();
CPU *smp_get_curr_cpu(int force);
STATUS cpu_set_errno(int64_t errno);
int64_t cpu_get_errno();
void init_tss(CPU *cpu_info);
void smp_init();