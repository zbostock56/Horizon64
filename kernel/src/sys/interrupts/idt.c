#include <idt.h>

void idt_init_entry(int interrupt, void *base, uint16_t segment, uint8_t type) {
  g_idt[interrupt].segment_selector = segment;
  g_idt[interrupt].base_low = ((uint64_t)base) & 0xFFFF;
  g_idt[interrupt].base_mid = (((uint64_t)base) >> 16) & 0xFFFF;
  g_idt[interrupt].base_high = (((uint64_t)base) >> 32) & 0xFFFFFFFF;
  g_idt[interrupt].ist = 0;
  g_idt[interrupt].type = type;
  /* _reserved already set to zero from memset() */
}

void idt_init() {
  /* Zero out all IDT entries */
  for (size_t i = 0; i < X86_64_IDT_ENTRIES; i++) {
    memset(&g_idt[i], 0, sizeof(IDT_ENTRY));
  }

  /* Load the IDT */
  asm volatile("lidt %0" : : "m"(g_idt_descriptor));
  terminal_printf(&term, "Finished loading IDT\n");
}

void idt_enable_gate(int interrupt) {
  /* Set the present bit */
  SET(g_idt[interrupt].type, IDT_FLAG_PRESENT);
}

void idt_disable_gate(int interrupt) {
  /* Zero the present bit */
  UNSET(g_idt[interrupt].type, IDT_FLAG_PRESENT);
}
