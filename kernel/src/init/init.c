/**
 * @file init.c
 * @author Zack Bostock
 * @brief Sets up system vitals
 * @verbatim
 * Sets up GDT, IDT, ISR, IRQ, and other important system variables.
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <init/init.h>

/**
 * @brief Main system initialization function where high level handlers
 *        are called.
 */
void system_init() {
    /* Intialize serial bus */
    serial_init();

    /* Initialize logging */
    klog_init();

    /* Initialize interrupt descriptor table */
    idt_init();

    /* Initialize interrupt service routines */
    isr_init();

    /* Initialize CPU specific features */
    cpu_init(0);

    /* Initialize global descriptor table */
    gdt_init(NULL);

    /* Initialize PIC, PIT */
    irq_init();

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

    /* Memory initialization */
    pm_init(mem_req);
    vm_init(mem_req, kernel_addr_request);


    /* Set up .psf1 font */
    psf1_font_init(file_request, "zap-vga16.psf");

    /* Initialize terminal */
    if (!framebuffer_req.response) {
        halt();
    }
    struct limine_framebuffer *fb = framebuffer_req.response->framebuffers[0];
    init_terminal(fb);

    /* Initialize terminal */
    terminal_start();

    /* Indicate the memory usage after virtual memory has been initialized */
    klogd("SYSTEM INIT: Memory used after initial mapping\n");
    pm_used();

    /* Initialize keyboard driver */
    keyboard_init();

    /* ACPI (and MADT) initialization */
    acpi_init(rsdp_request);

    /* High Precision Event Timer (HPET) initialization */
    hpet_init();

    /* Intialize CMOS/RTC */
    cmos_init();

    /* Intialize PCI device list */
    pci_init();
    
    /* Initialize Advanced Programmable Interrupt Controller */
    apic_init();

    /* Initalize Symmetric Multi-Processing */
    smp_init();

    /* Initalize system calls for the Bootstrap Processor (BSP) */
    system_calls_init();

    /* Initialize virtual filesystem */
    vfs_init();

    /* Load and initialize Initial Ram Disk */
    initrd_init(file_request);

    klogs("SYSTEM INIT: System initialized successfully...\n");
}
