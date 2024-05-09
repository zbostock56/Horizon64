#include <asm.h>
#include <globals.h>

void outb(int16_t port, uint8_t val) {
__asm__ volatile ("outb %b0, %w1"
                  :
                  : "a"(val), "Nd"(port)
                  : "memory");
}

uint8_t inb(uint16_t port) {
  uint8_t ret;
  __asm__ volatile ("inb %w1, %b0"
                    : "=a"(ret)
                    : "Nd"(port)
                    : "memory");
  return ret;
}

void io_wait() {
  outb(0x80, 0);
}

__attribute__((noreturn)) void halt() {
  disable_interrupts();
    __asm__ volatile("hlt");
    for(;;);
}

void disable_interrupts() {
  __asm__ volatile("cli");
}

void enable_interrupts() {
  __asm__ volatile("sti");
}