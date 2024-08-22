/**
 * @file fadt_str.h
 * @author Zack Bostock
 * @brief Fixed ACPI Description Table (FADT) contains information about ACPI
 * fixed registers, DSDT pointer, etc. all pertaining to power management.
 * @ref https://wiki.osdev.org/FADT
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>

#include <structs/rsdt_str.h>
#include <structs/generic_addr_structure_str.h>

typedef struct {
    ACPI_SDT_HEADER header;
    uint32_t firmware_ctrl;
    uint32_t dsdt;

    /* Filed used in ACPI 1.0; no longer in use, for compatibility only */
    uint8_t reserved;

    uint8_t preferred_power_management_profile;
    uint16_t sci_interrupt;
    uint32_t ami_command_port;
    uint8_t acpi_enable;
    uint8_t s4_bios_req;
    uint8_t pstate_control;
    uint32_t pm1a_event_block;
    uint32_t pm1b_event_block;
    uint32_t pm1a_control_block;
    uint32_t pm1b_control_block;
    uint32_t pm2_control_block;
    uint32_t pm_timer_block;
    uint32_t gpe0_block;
    uint32_t gpe1_block;
    uint8_t pm1_event_length;
    uint8_t pm1_control_length;
    uint8_t pm2_control_length;
    uint8_t pm_timer_length;
    uint8_t gpe0_length;
    uint8_t gpe1_length;
    uint8_t gpe1_base;
    uint8_t c_state_control;
    uint16_t worst_c2_latency;
    uint16_t worst_c3_latency;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t duty_offset;
    uint8_t duty_width;
    uint8_t day_alarm;
    uint8_t month_alarm;
    uint8_t century;

    /* reserved in ACPI 1.0; used since ACPI 2.0+ */
    uint16_t boot_architechture_flags;

    uint8_t reserved2;
    uint32_t flags;

    GENERIC_ADDR_STRUCT reset_reg;

    uint8_t reset_value;
    uint8_t reserved3[3];

    /* 64-bit pointers - available on ACPI 2.0+ */
    uint64_t x_firmware_control;
    uint64_t x_dsdt;

    GENERIC_ADDR_STRUCT x_pm1a_event_block;
    GENERIC_ADDR_STRUCT x_pm1b_event_block;
    GENERIC_ADDR_STRUCT x_pm1a_control_block;
    GENERIC_ADDR_STRUCT x_pm1b_control_block;
    GENERIC_ADDR_STRUCT x_pm2_control_block;
    GENERIC_ADDR_STRUCT x_pm_timer_block;
    GENERIC_ADDR_STRUCT x_gpe0_block;
    GENERIC_ADDR_STRUCT x_gpe1_block;
} FADT;
