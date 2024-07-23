/**
 * @file globals.h
 * @author Zack Bostock
 * @brief Global scope initialization area for global variables
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <const.h>
#include <limine.h>
#include <structs/psf_structs.h>
#include <structs/framebuffer_str.h>
#include <structs/terminal_str.h>

/* Used to as a status message response after function completion */
typedef uint8_t STATUS;

/* Framebuffers */
extern FRAMEBUFFER initial_fb;

/* Terminals */
extern TERMINAL term;

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

