/**
 * @file acpi.h
 * @author Zack Bostock
 * @brief ACPI (Advanced Configuration and Power Interface) related information
 * @verbatim
 * ACPI is a power management and configuration standard for PC, developed by
 * Intel, Microsoft, and Toshiba. It allows the OS to control the amount of
 * power each device is given, allowing it to put certain devices on standby
 * or power-off. It is also used to control and/or check thermal zones, battery
 * levels, PCI IRQ routing, CPUs, NUMA domains, and many other things.
 *
 * Information about ACPI is stored in the BIOS's memory (for those which
 * support it, of course).
 *
 * There are 2 main parts. The first part is the tables used by the OS for
 * configuration during boot. The second part is the run time ACPI environment,
 * which consists of platform independent OOP code which comes from the BIOS,
 * called AML code, and the ACPI System Management Mode (SMM) code.
 * @ref https://wiki.osdev.org/ACPI
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stddef.h>

#include <globals.h>

#include <common/string.h>

#include <structs/acpi_str.h>

#include <sys/mmu.h>
#include <sys/acpi/madt.h>
#include <sys/acpi/rsdp.h>
#include <sys/asm.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
ACPI_SDT *acpi_get_sdt(const char *signature);
void acpi_init(LIMINE_RSDP_REQ req);
