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
#define KLOG_LVL_DEBUG      (1)
#define KLOG_LVL_INFO       (2)
#define KLOG_LVL_WARN       (3)
#define KLOG_LVL_ERROR      (4)
#define KLOG_LVL_TAB        (5)
#define KLOG_LVL_NONE       (6)
#define KLOG_LVL_SRTUP      (7)
#define KLOG_LVL_UNKNOWN    (8)

#define KLOG_BUFFER_SIZE    (UINT16_MAX + 1)

typedef struct {
  uint8_t buffer[UINT16_MAX];
  int start;
  int end;
  TERM_MODE mode;
} KLOG;

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void klog_init();
void kputc(TERM_MODE mode, uint8_t c);
void kputs(TERM_MODE mode, const char *s, int width);
void klog_refresh(TERM_MODE mode);
void klog_vprintf(uint8_t level, const char *s, ...);
void kprintf(const char *format, ...);
void klog_lock();
void klog_unlock();

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