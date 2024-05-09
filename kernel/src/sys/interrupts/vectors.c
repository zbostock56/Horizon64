#include <globals.h>

/*
  Keeps the tick of the kernel. Handles scheduling and keeping time.
*/
void clkhandler() {
  system_time++;
  if (--preempt_quantum <= 0) {
    preempt_quantum = QUANTUM;
    /* TODO: Implement scheduling */
    /* Reschedule */
  }
}