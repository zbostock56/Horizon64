#include "idt.h"

#define SET(x, flag) (x |= (flag))
#define UNSET(x, flag) (x &= ~(flag))

IDT_ENTRY g_idt[i686_IDT_ENTRIES];
IDT_DESCRIPTOR g_idt_descriptor = {
    sizeof(g_idt) - 1,
    g_idt
};

/* Assembly function which carries out the setting of the IDT values */
void __attribute__((cdecl)) i686_idt_load(IDT_DESCRIPTOR *);

void i686_idt_set_gate(int interrupt, void *base, uint16_t segement_descriptor, uint8_t flags) {
    g_idt[interrupt].base_low = ((uint32_t) base) & 0xFFFF;
    g_idt[interrupt].segment_selector = segement_descriptor;
    g_idt[interrupt]._reserved = 0;
    g_idt[interrupt].flags = flags;
    g_idt[interrupt].base_high = ((uint32_t) base >> 16) & 0xFFFF;
}

void i686_idt_enable_gate(int interrupt) {
    SET(g_idt[interrupt].flags, IDT_FLAG_PRESENT);
}

void i686_idt_disable_gate(int interrupt) {
    UNSET(g_idt[interrupt].flags, IDT_FLAG_PRESENT);
}

void i686_idt_init() {
    i686_idt_load(&g_idt_descriptor);
}