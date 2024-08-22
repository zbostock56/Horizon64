/**
 * @file init.h
 * @author Zack Bostock
 * @brief Information pertaining to initialization of the kernel
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <globals.h>

#include <dev/terminal.h>
#include <dev/keyboard/keyboard.h>
#include <dev/serial.h>

#include <sys/acpi/acpi.h>
#include <sys/cpu.h>
#include <sys/gdt/gdt.h>
#include <sys/mmu.h>
#include <sys/interrupts/isr.h>
#include <sys/interrupts/idt.h>
#include <sys/interrupts/irq.h>

#include <init/psf.h>
#include <init/boot_info.h>

#include <graphics/framebuffer.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */
static volatile LIMINE_BASE_REVISION(2);
static volatile struct limine_module_request psf_file_request = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0
};

static volatile struct limine_framebuffer_request framebuffer_req = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

static volatile struct limine_memmap_request mem_req = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

static volatile struct limine_kernel_address_request kernel_addr_request = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0
};

static volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0,
};

static volatile struct limine_bootloader_info_request bl_info_req = {
    .id = LIMINE_BOOTLOADER_INFO_REQUEST,
    .revision = 0,
};

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void system_init();

/* --------------------------- EXTERNALLY DEFINED --------------------------- */
void init_terminal(FRAMEBUFFER fb);
