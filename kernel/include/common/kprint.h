/**
 * @file kprint.h
 * @author Zack Bostock
 * @brief Information pertaining to kernel logging/printing
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <globals.h>
#include <stdarg.h>

#include <structs/terminal_str.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
#define KLOG_LVL_VERBOSE    (0)
#define KLOG_LVL_TAB        (1)
#define KLOG_LVL_NONE       (2)
#define KLOG_LVL_DEBUG      (3)
#define KLOG_LVL_INFO       (4)
#define KLOG_LVL_WARN       (5)
#define KLOG_LVL_ERROR      (6)
#define KLOG_LVL_SRTUP      (7)
#define KLOG_LVL_UNKNOWN    (8)

#define KLOG_BUFFER_SIZE    (0x10000UL)

typedef struct {
  uint8_t buffer[KLOG_BUFFER_SIZE];
  unsigned long start;
  unsigned long end;
  TERMINAL *term;
} KLOG;

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void klog_init();
void klog_refresh(TERM_MODE mode);
void klog_vprintf(uint8_t level, const char *s, ...);
void kprintf(const char *format, ...);
void klog_lock();
void klog_unlock();
void klog_toggle_print_prefix(int toggle);

/* --------------------------------- MACROS --------------------------------- */
#define klogi(s, ...)   klog_vprintf(KLOG_LVL_INFO, s, ##__VA_ARGS__)
#define klogd(s, ...)   klog_vprintf(KLOG_LVL_DEBUG, s, ##__VA_ARGS__)
#define klogv(s, ...)   klog_vprintf(KLOG_LVL_VERBOSE, s, ##__VA_ARGS__)
#define klogw(s, ...)   klog_vprintf(KLOG_LVL_WARN, s, ##__VA_ARGS__)
#define kloge(s, ...)   klog_vprintf(KLOG_LVL_ERROR, s, ##__VA_ARGS__)
#define klogt(s, ...)   klog_vprintf(KLOG_LVL_TAB, s, ##__VA_ARGS__)
#define klogn(s, ...)   klog_vprintf(KLOG_LVL_NONE, s, ##__VA_ARGS__)
#define klogu(s, ...)   klog_vprintf(KLOG_LVL_UNKNOWN, s, ##__VA_ARGS__)
#define klogs(s, ...)   klog_vprintf(KLOG_LVL_SRTUP, s, ##__VA_ARGS__)

#define klog(s, ...)    klog_vprintf(KLOG_LVL_INFO, s, ##__VA_ARGS__)