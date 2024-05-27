#pragma once

#include <stdint.h>
#include <stddef.h>
#include <const.h>
#include <limine.h>
#include <sys/asm.h>
#include <structs/psf_structs.h>
#include <structs/idt_str.h>
#include <structs/framebuffer_str.h>
#include <structs/gdt_str.h>
#include <structs/regs_str.h>
#include <structs/terminal_str.h>
#include <graphics/graphics.h>

typedef struct limine_file LIMINE_FILE;
typedef struct limine_module_response LIMINE_MODULE_RESP;
typedef struct limine_module_request LIMINE_MODULE_REQ;
typedef struct limine_framebuffer_request LIMINE_FRAMEBUFF_REQ;
typedef struct limine_memmap_request LIMINE_MEM_REQ;

/* Used to as a status message response after function completion */
typedef uint8_t STATUS;

/* System time and system timer information */
extern volatile uint64_t system_time;
extern volatile uint8_t preempt_quantum;

/* Hardware Interrupts */
extern IRQ_HANDLER g_irq_handler[NUM_HARDWARE_INTERRUPTS];

/* Framebuffers */
extern FRAMEBUFFER initial_fb;

/* Terminals */
extern TERMINAL term;

/* Interrupt descriptor table */
extern IDT_ENTRY g_idt[X86_64_IDT_ENTRIES];
extern IDT_DESCRIPTOR g_idt_descriptor;

/* Global descriptor table */
extern GDT_TABLE g_gdt[NUM_CPUS];
extern size_t num_gdt;

/* Interrupt service routines */
extern ISR_HANDLER g_isr_handlers[X86_64_IDT_ENTRIES];

/* PSF1 FONT */
extern PSF1_FONT font;

/* Global use functions */
void kprintf(const char *format, ...);
void klog_implementation(int level, const char *format, ...);
// #define klogi(x, ...) klog_implementation(LEVEL_LOG, x, ##__VA_ARGS__)
#define klogi(x, ...) kprintf(x, ##__VA_ARGS__)
#define kloge(x, ...) klog_implementation(LEVEL_ERROR, x, ##__VA_ARGS__)
#define klogd(x, ...) klog_implementation(LEVEL_DEBUG, x, ##__VA_ARGS__)

void kputc(char c);
void terminal_printf(TERMINAL *t, const char *format, ...);

