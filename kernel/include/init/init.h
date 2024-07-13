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

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* ----------------------------- STATIC GLOBALS ----------------------------- */
static volatile LIMINE_BASE_REVISION(1);
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

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void system_init();

/* --------------------------- EXTERNALLY DEFINED --------------------------- */
void gdt_init(/* CPU *cpu_info */);
void cpu_init(size_t cpu_number);
void isr_init();
void idt_init();
void psf1_font_init(struct limine_module_request req, const char *path);
void fb_init(struct limine_framebuffer_request req);
void init_terminal(FRAMEBUFFER fb);
void irq_init();
void keyboard_init();
void pm_init(LIMINE_MEM_REQ req);