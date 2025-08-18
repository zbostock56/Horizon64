/**
 * @file lock.c
 * @author Zack Bostock
 * @brief Hardware locking functionality
 * @verbatim
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <kconfig.h>
#include <structs/lock_str.h>
#include <common/string.h>
#include <common/memory.h>
#include <common/kprint.h>

/**
 * @brief Locks a hardware lock
 * @verbatim
 * Locks a hardware lock which can be used to ensure no other process is
 * able to perform this operation while another already is.
 *
 * @param s LOCK structure
 * @param f File name
 * @param ln Line number
 */
void lock_lock_implementation(LOCK *s, const char *f, const int ln) {

#if LOCK_DEBUG
    if (s->lock) {
        klog_emergency_printf("Trying to lock an already locked lock (%s:%d, %s:%d)\n!",
                                s->fn, s->line, f, ln);
    }
    memset(s->fn, 0, LOCK_FN_LENGTH);
    strncpy(s->fn, f, strlen(f));
    s->line = ln;
#else
    (void) f;
    (void) ln;
#endif

  asm __volatile__ (
      "pushfq;"
      "cli;"
      "lock;"
      "btsl $0, %[lock];"
      "jnc 2f;"
      "1:"
      "pause;"
      "btl $0, %[lock];"
      "jc 1b;"
      "lock btsl $0, %[lock];"
      "jc 1b;"
      "2:"
      "pop %[flags]"
      : [lock] "=m"((s)->lock), [flags] "=m"((s)->rflags)
      :
      : "memory", "cc");
}

/**
 * @brief Unlock hardware lock
 *
 * @param s LOCK structure
 * @param f File name
 * @param ln Line number
 */
void unlock_lock_implementation(LOCK *s, const char *f, const int ln) {

#if LOCK_DEBUG
    if (strcmp(s->fn, f) && s->lock) {
        klog_emergency_printf("UNLOCK_LOCK: Trying to unlock lock which isn't owned!"
                              " (%s:%d, %s:%d)\n",
                              s->fn, s->line, f, ln);
    } else if (!s->lock) {
        klog_emergency_printf("UNLOCK_LOCK: Trying to unlock an already unlocked lock! (%s:%c)\n",
                                f, ln);
    }
#else
    (void) f;
    (void) ln;
#endif

    asm __volatile__ (
                    "push %[flags];"
                    "lock;"
                    "btrl $0, %[lock];"
                    "popfq;"
                    : [lock] "=m"((s)->lock)
                    : [flags] "m"((s)->rflags)
                    : "memory", "cc");
}
