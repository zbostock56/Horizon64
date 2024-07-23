/**
 * @file idt.c
 * @author Zack Bostock
 * @brief Initialization for the Interrupt Descriptor Table
 * @verbatim
 * In this file is the initialization code and helpers for the Interrupt
 * Descriptor Table (IDT).
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <sys/interrupts/idt.h>

/* Interrupt descriptor table */
__attribute__((aligned(0x10))) static IDT_ENTRY g_idt[X86_64_IDT_ENTRIES] = {0};
IDT_DESCRIPTOR g_idt_descriptor = {
    sizeof(g_idt) - 1,
    (uint64_t) g_idt
};

/**
 * @brief Helper to create IDT entries.
 *
 * @param interrupt Entry number
 * @param base Base in memory
 * @param segment Segment in memory
 * @param type Type of gate
 */
void idt_init_entry(int interrupt, void *base, uint16_t segment, uint8_t type) {
  g_idt[interrupt].segment_selector = segment;
  g_idt[interrupt].base_low = ((uint64_t)base) & 0xFFFF;
  g_idt[interrupt].base_mid = (((uint64_t)base) >> 16) & 0xFFFF;
  g_idt[interrupt].base_high = (((uint64_t)base) >> 32) & 0xFFFFFFFF;
  g_idt[interrupt].ist = 0;
  g_idt[interrupt].type = type;
  /* _reserved already set to zero from memset() */
}

/**
 * @brief Main initialization function for the IDT.
 */
void idt_init() {
  /* Zero out all IDT entries */
  for (size_t i = 0; i < X86_64_IDT_ENTRIES; i++) {
    memset(&g_idt[i], 0, sizeof(IDT_ENTRY));
  }

  /* Load the IDT */
  asm volatile("lidt %0" : : "m"(g_idt_descriptor));
  klogi("Finished loading IDT\n");
}

/**
 * @brief Enables an interrupt in the IDT.
 *
 * @param interrupt Interrupt to enable
 */
void idt_enable_gate(int interrupt) {
  /* Set the present bit */
  SET(g_idt[interrupt].type, IDT_FLAG_PRESENT);
}

/**
 * @brief Disables an interrupt in the IDT.
 *
 * @param interrupt Interrupt to disable
 */
void idt_disable_gate(int interrupt) {
  /* Zero the present bit */
  UNSET(g_idt[interrupt].type, IDT_FLAG_PRESENT);
}
