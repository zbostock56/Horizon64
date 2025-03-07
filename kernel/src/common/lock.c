/**
 * @file lock.c
 * @author Zack Bostock
 * @brief Hardware locking functionality
 * @verbatim
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <structs/lock_str.h>


#if 1
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
  (void) f;
  (void) ln;

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
  (void) f;
  (void) ln;

  asm __volatile__ (
                "push %[flags];"
                "lock;"
                "btrl $0, %[lock];"
                "popfq;"
                : [lock] "=m"((s)->lock)
                : [flags] "m"((s)->rflags)
                : "memory", "cc");
}

#endif

#if 0
/**
 * @brief Locks a hardware lock
 *
 * Locks a hardware lock to ensure no other process can perform the operation while another already is.
 *
 * @param s LOCK structure
 * @param f File name
 * @param ln Line number
 */
void lock_lock_implementation(LOCK *s, const char *f, const int ln) {
    (void) f;
    (void) ln;

    asm __volatile__ (
        "cli;"                  // Disable interrupts
        "lock btsl $0, %[lock];" // Attempt to set the lock bit atomically
        "jnc 2f;"               // If lock was free, jump to acquired section
        "1:"                    // Contention, spin here
        "pause;"                // Pause instruction for power efficiency
        "btl $0, %[lock];"      // Check if the lock is still held
        "jc 1b;"                // If held, continue spinning
        "lock btsl $0, %[lock];"// Try to acquire the lock again
        "jc 1b;"                // If still held, spin
        "2:"                    // Acquired, fall through
        "mfence;"               // Ensure proper memory ordering
        : [lock] "=m"((s)->lock)
        :
        : "memory", "cc"
    );
    // Interrupts disabled, lock acquired
}

/**
 * @brief Unlocks the hardware lock
 *
 * @param s LOCK structure
 * @param f File name
 * @param ln Line number
 */
void unlock_lock_implementation(LOCK *s, const char *f, const int ln) {
    (void) f;
    (void) ln;

    asm __volatile__ (
        "mfence;"               // Ensure all previous memory ops complete
        "lock btrl $0, %[lock];" // Clear the lock bit atomically
        "sti;"                  // Re-enable interrupts
        : [lock] "=m"((s)->lock)
        :
        : "memory", "cc"
    );
}
#endif