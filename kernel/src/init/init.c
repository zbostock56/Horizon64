/**
 * @file init.c
 * @author Zack Bostock
 * @brief Sets up system vitals
 * @verbatim
 * Sets up GDT, IDT, ISR, IRQ, and other important system variables.
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <init/init.h>

/**
 * @brief Main system initialization function where high level handlers
 *        are called.
 */
void system_init() {
    klogi("SYSTEM INIT: Starting...\n");

    /* Initial Limine check */
    if (!LIMINE_BASE_REVISION_SUPPORTED) {
        kloge("SYSTEM INIT: Base revision not supported!\n");
        halt();
    }

    /* Print information about the bootloader */
    if (print_boot_info(bl_info_req) == SYS_ERR) {
        halt();
    }

    /* Print information about the system image */
    if (hhdm_request.response) {
      klogi("SYSTEM INIT: HHDM offset %x, revision %d\n",
      hhdm_request.response->offset, hhdm_request.response->revision);
    }

    /* Intialize serial bus */
    if (serial_init() == SYS_ERR) {
        kloge("SYSTEM INIT: Serial initialization failed or serial faulty!\n");
    }

    /* Initialize global descriptor table */
    gdt_init();

    /* Initialize interrupt descriptor table */
    idt_init();

    /* Initialize CPU specific features */
    cpu_init(0);

    /* Initialize interrupt service routines */
    isr_init();

    /* Memory initialization */
    pm_init(mem_req);
    vm_init(mem_req, kernel_addr_request);

    /* Indicate the memory usage after virtual memory has been initialized */
    klogi("SYSTEM INIT: Memory used after initial mapping\n");
    pm_used();

    /* ACPI (and MADT) initialization */
    acpi_init(rsdp_request);

    /* High Precision Event Timer (HPET) initialization */
    hpet_init();

    /* Intialize CMOS/RTC */
    cmos_init();

    /* Intialize PCI device list */
    pci_init();

    /* Initialize framebuffer */
    fb_init(framebuffer_req);

    /* Set up .psf1 font */
    psf1_font_init(psf_file_request, "zap-vga16.psf");

    /* Initialize terminal */
    init_terminal(initial_fb);

    /* Initialize PIC, PIT */
    irq_init();

    /* Initialize keyboard driver */
    keyboard_init();

    klogi("SYSTEM INIT: System initialized successfully...\n");
}
