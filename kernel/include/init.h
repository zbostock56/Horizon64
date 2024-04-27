#pragma once

#include <globals.h>

static volatile LIMINE_BASE_REVISION(1);
static volatile struct limine_module_request psf_file_request = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0
};

static volatile struct limine_framebuffer_request framebuffer_req = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void system_init();

/* --------------------------- EXTERNALLY DEFINED --------------------------- */
void get_iso_file(const char *name, LIMINE_MODULE_REQ module_request,
                  LIMINE_FILE **file);
int psf1_get_glyphs(LIMINE_FILE *file, PSF1_FONT **font);
void gdt_init(/* CPU *cpu_info */);
void isr_init();
void idt_init();