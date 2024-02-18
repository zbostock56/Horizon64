#include "kernel.h"
#include <arch/i686/general.h>

/*
  Handles IRQ0 (Programmable interval timer) interrupt
*/
void clkhandler(REGISTERS *regs) {
  system_time++;
  if ((--preempt_quantum) == 0) {
    preempt_quantum = QUANTUM;
  }
}