#include <common/lock.h>

/*
  Locks a hardware lock which can be used to ensure no other process is
  able to perform this operation while another already is.
*/
void lock_lock_implementation(LOCK *s, const char *f, const int ln) {
  (void) f;
  (void) ln;

  asm __volatile__ (
      "pushfq;"
      "cli;"
      "lock btsl $0, %[lock];"
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

/*
  Relinquishes control of the hardware lock.
*/
void unlock_lock_implmentation(LOCK *s, const char *f, const int ln) {
  (void) f;
  (void) ln;

  asm __volatile__ (
                "push %[flags];"
                "lock btrl $0, %[lock];"
                "popfq;"
                : [lock] "=m"((s)->lock)
                : [flags] "m"((s)->rflags)
                : "memory", "cc");
}