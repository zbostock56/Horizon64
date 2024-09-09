/**
 * @file apic.h
 * @author Zack Bostock
 * @brief Information pertaining to the APIC
 * @verbatim 
 * The Advanced Programmable Interrupt Controller (APIC) is the updated Intel
 * standard for the older Programmable Interrupt Controller (PIC). It is used
 * in multiprocessor systems and is an integral part of all recent Intel (and
 * compatible) processors. The APIC is used for sophisticated interrupt
 * redirection, and for sending interrupts between processors. These things
 * weren't possible using the older PIC specification.
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <const.h>

#include <sys/cpu.h>
#include <sys/cpu_features.h>
#include <sys/mmu.h>
#include <sys/acpi/madt.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
/**
 * @brief Local APIC registers
 */
#define APIC_LAPIC_ID_REG                    (0x020)
#define APIC_LAPIC_VERSION_REG               (0x030)
#define APIC_TASK_PRIORITY_REG               (0x080)
#define APIC_ARBITRATION_PRIORITY_REG        (0x090)
#define APIC_PROCESSOR_PRIORITY_REG          (0x0A0)
#define APIC_EOI_REG                         (0x0B0)
#define APIC_REMOTE_READ_REG                 (0x0C0)
#define APIC_LOGICAL_DEST_REG                (0x0D0)
#define APIC_DEST_FORMAT_REG                 (0x0E0)
#define APIC_SPURIOUS_INT_VEC_REG            (0x0F0)
#define APIC_ISR_START_REG                   (0x100)
#define APIC_TRIGGER_MODE_START_REG          (0x180)
#define APIC_ERROR_STATUS_REG                (0x280)
#define APIC_CMCI_REG                        (0x2F0)
#define APIC_INT_CMD_LOW_REG                 (0x300)
#define APIC_INT_CMD_HIGH_REG                (0x310)
#define APIC_LVT_TMR_REG                     (0x320)
#define APIC_LVT_THERMAL_SENSOR_REG          (0x330)
#define APIC_LVT_PERF_MONITORING_REG         (0x340)
#define APIC_LVT_LINT0_REG                   (0x350)
#define APIC_LVT_LINT1_REG                   (0x360)
#define APIC_LVT_ERR_REG                     (0x370)
#define APIC_INIT_COUNT_REG                  (0x380)
#define APIC_CURRENT_COUNT_REG               (0x390)
#define APIC_DIVIDE_CONFIG_REG               (0x3E0)

/**
 * @brief Spurious interrupt vector number
 * @note Sets the spurious interrupt number to IRQ7, assuming this constant
 *       is 0x027. This is per the OSDev Wiki
 */
#define APIC_SPURIOUS_INT_NUM                (0x027)

/**
 * @brief Constants related to Inter-Processor Interrupts
 */
#define APIC_IPI_MTYPE_INIT                  (0x005)
#define APIC_IPI_MTYPE_STARTUP               (0x006)

/**
 * @brief Constants that were defined at https://wiki.osdev.org/APIC
 */
#define IA32_APIC_BASE_MSR                   (0x01B)
#define IA32_APIC_BASE_MSR_BSP               (0x100) // Processor is a BSP
#define IA32_APIC_BASE_MSR_ENABLE            (0x800)
#define IA32_APIC_BASE_MSR_X2APIC            (0x400)
#define APIC_ENABLE                          (0x100)
#define APIC_TIMER_MASKED                    (1 << 16)
#define APIC_TIMER_PERIODIC                  (1 << 17)

typedef enum {
    APIC_ONE_SHOT_MODE,
    APIC_PERIODIC_MODE
} APIC_TMR_MODE;

/**
 * @brief Constant which defines what hardware interrupt to use for the APIC
 * @note This number is from a base of the zeroth interrupt. Meaning, the number
 *       does not have an offset.
 */
#define APIC_HW_INT_NUM                      (32)

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */
/**
 * @brief Helper macro for getting at each of the ISR registers
 */
#define ISR_REG_N(num) (ISR_START_REG + (num * 0x010))

/**
 * @brief Helper macro for getting at each of the Trigger Mode registers
 */
#define TRIGGER_MODE_REG_N(num) (TRIGGER_MODE_START_REG + (num * 0x010))

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void apic_init();
void apic_send_end_of_interrupt();
void apic_timer_init();
uint8_t apic_timer_int_is_delivered();