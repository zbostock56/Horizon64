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
void gdt_init(/* CPU *cpu_info */);
void isr_init();
void idt_init();
void psf1_font_init(struct limine_module_request req, const char *path);
void fb_init(struct limine_framebuffer_request req);
void init_terminal(FRAMEBUFFER fb);
void irq_init();
void isr_register_handler(int interrupt, ISR_HANDLER handler);