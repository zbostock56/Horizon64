#pragma once

#include <globals.h>
#include <stdarg.h>
// #include <common/lock.h>

typedef struct {
  uint8_t buffer[UINT16_MAX];
  int start;
  int end;
  TERMINAL_MODE mode;
} KLOG;

/* ----------------------------- STATIC GLOBALS ----------------------------- */
static const char CONVERSION_TABLE[] = "0123456789abcdef";
// static LOCK klog_shackle = {0};
// static KLOG klog = {0};

/* --------------------------------- DEFINES -------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void kprintf(const char *format, ...);
void klog_implementation(int level, const char *format, ...);

/* --------------------------- EXTERNALLY DEFINED --------------------------- */