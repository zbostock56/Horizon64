/**
 * @file apic.c
 * @author your name (you@domain.com)
 * @brief Functionality pertaining to the APIC
 * @verbatim
 * The Advanced Programmable Interrupt Controller (APIC) is the updated Intel
 * standard for the older Programmable Interrupt Controller (PIC). It is used
 * in multiprocessor systems and is an integral part of all recent Intel (and
 * compatible) processors. The APIC is used for sophisticated interrupt
 * redirection, and for sending interrupts between processors. These things
 * weren't possible using the older PIC specification.
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <sys/acpi/apic.h>
#include <sys/tick/pic.h>
#include <sys/tick/clkhandler.h>
#include <sys/interrupts/irq.h>
#include <sys/interrupts/isr.h>

#include <sys/gdt/gdt.h>

/* Globals related to the APIC initialization */
static int two_acpi_enabled = 0;
volatile void *local_apic_base = NULL;

/* Globals related to APIC as the system timer */
static uint8_t apic_timer_enabled = 0;
static uint64_t base_frequency = 0;
static uint8_t divisor = 0;
static uint8_t avail_apic_isr_vector = 0;

/* -------- GENERAL FUNCTIONS RELATED TO INITIALIZATION OF THE APIC -------- */

/**
 * @brief Helper function for determining if the APIC exists on this processor
 *
 * @return STATUS SYS_OK if yes, SYS_ERR otherwise
 */
STATUS check_apic_exists() {
    return check_cpu_support(cpuid_feature_apic);
}

/**
 * @brief Helper function for determining if the 2x APIC exists on this
 *        processor
 *
 * @return STATUS SYS_OK if yes, SYS_ERR otherwise
 */
STATUS check_2xapic_exists() {
    return check_cpu_support(cpuid_feature_2xapic);
}

/**
 * @brief Helper for writing to an APIC register
 *
 * @param offset Register to write to
 * @param value Value to write to register
 */
static inline void apic_write_reg(uint16_t offset, uint32_t value) {
    if (!local_apic_base) {
        return;
    }
    // cppcheck-suppress arithOperationsOnVoidPointer
    *(uint32_t volatile *)(local_apic_base + offset) = value;
}

/**
 * @brief Helper for reading an APIC register (static inline version)
 *
 * @param offset Register to read from
 * @return uint32_t Value from the register
 */
static inline uint32_t static_apic_read_reg(uint16_t offset) {
    if (!local_apic_base) {
        return -1;
    }
    return *(uint32_t volatile *)(local_apic_base + offset);
}

/**
 * @brief Helper for reading APIC register (public version)
 *
 * @param offset Register to read from
 * @return uint32_t Value from the register
 */
uint32_t apic_read_reg(uint64_t offset) {
    if (!local_apic_base) {
        return -1;
    }

    return *(uint32_t volatile *)(local_apic_base + offset);
}

/**
 * @brief Helper to send the end of interrupt signal to the APIC
 * @note Send value 0 to signal an end of interrupt. A non-zero value may
 *       cause #GP
 */
void apic_send_end_of_interrupt() {
    apic_write_reg(APIC_EOI_REG, 0);
}

/**
 * @brief Sends the Inter-Processor Interrupt to a specific processor
 *
 * @param processor Destination processor
 * @param vector Interrupt vector to use
 * @param mtype The message type for the Inter-Processor Interrupt
 */
void apic_send_ipi(uint8_t processor, uint8_t vector, uint32_t mtype) {
    apic_write_reg(APIC_INT_CMD_HIGH_REG, (uint32_t) processor << 24);
    apic_write_reg(APIC_INT_CMD_LOW_REG, (mtype << 8) | vector);
}

/**
 * @brief Resets the values in the error register if about to do operations with
 *        APIC initialization. The read back should be 0 when writing 1.
 *
 * @return uint8_t SYS_OK if success, SYS_ERR otherwise
 */
STATUS apic_reset_error_reg() {
    apic_write_reg(APIC_ERROR_STATUS_REG, 0x1);
    return !((static_apic_read_reg(APIC_ERROR_STATUS_REG) >> 1) & 0x1);
}

/**
 * @brief Helper for reading values from the error register
 */
void apic_check_error_reg() {
    klogd("APIC ERROR CHECK:\n");
    uint32_t value = static_apic_read_reg(APIC_ERROR_STATUS_REG);
    /*
        Send Checksum Error:
        Set when the local APIC detects a checksum error for a message that
        it sent on the APIC bus. Used only on P6 family and Pentium processors.
    */
    klogd("\tChecking for send checksum error... ");
    if ((value >> 0) & 0x1) {
#if ENABLE_KLOG_DEBUG
        klogn("error\n");
#endif
        goto error;
    }
#if ENABLE_KLOG_DEBUG
    klogn("success\n");
#endif

    /*
        Receive Checksum Error:
        Set when the local APIC detects a checksum error for a message that
        it received on the APIC bus. Used only on P6 family and Pentium processors.
    */
    klogd("\tChecking for receive checksum error... ");
    if ((value >> 1) & 0x1) {
#if ENABLE_KLOG_DEBUG
        klogn("error\n");
#endif
        goto error;
    }
#if ENABLE_KLOG_DEBUG
    klogn("success\n");
#endif

    /*
        Send Accept Error:
        Set when the local APIC detects that a message it sent was not accepted
        by any APIC on the APIC bus. Used only on P6 family and Pentium processors.
    */
    klogd("\tChecking for send accept error... ");
    if ((value >> 2) & 0x1) {
#if ENABLE_KLOG_DEBUG
        klogn("error\n");
#endif
        goto error;
    }
#if ENABLE_KLOG_DEBUG
    klogn("success\n");
#endif

    /*
        Receive Accept Error:
        Set when the local APIC detects that a message it received was not accepted
        by any APIC on the APIC bus. Used only on P6 family and Pentium processors.
    */
    klogd("\tChecking for receive accept error... ");
    if ((value >> 3) & 0x1) {
#if ENABLE_KLOG_DEBUG
        klogn("error\n");
#endif
        goto error;
    }
#if ENABLE_KLOG_DEBUG
    klogn("success\n");
#endif

    /*
        Redirectable IPI Error:
        Set when the local APIC detects an attempt to send an IPI with the
        lowest-priority delivery mode and the local APIC does not support the
        sending of such IPIs. This bit is used on some Intel Core and Intel
        Xeon processors.
    */
    klogd("\tChecking for redirectable IPI error... ");
    if ((value >> 4) & 0x1) {
#if ENABLE_KLOG_DEBUG
        klogn("error\n");
#endif
        goto error;
    }
#if ENABLE_KLOG_DEBUG
    klogn("success\n");
#endif

    /*
        Send Illegal Vector Error:
        Set when the local APIC detects an illegal vector (one in the range 0
        to 15) in the message that it is sending. This occurs as the result of
        a write to the ICR (in both xAPIC and x2APIC modes) or to SELF IPI register
        (x2APIC mode only) with an illegal vector
    */
    klogd("\tChecking for send illegal vector error... ");
    if ((value >> 5) & 0x1) {
#if ENABLE_KLOG_DEBUG
        klogn("error\n");
#endif
        goto error;
    }
#if ENABLE_KLOG_DEBUG
    klogn("success\n");
#endif


    /*
        Receive Illegal Vector Error:
        Set when the local APIC detects an illegal vector (one in the range 0
        to 15) in an interrupt message it receives or in an interrupt generated
        locally from the local vector table or via a self IPI. Such interrupts are not delivered
        to the processor; the local APIC will never set an IRR bit in the range 0 to 15.
    */
    klogd("\tChecking for send illegal vector error... ");
    if ((value >> 6) & 0x1) {
#if ENABLE_KLOG_DEBUG
        klogn("error\n");
#endif
        goto error;
    }
#if ENABLE_KLOG_DEBUG
    klogn("success\n");
#endif

    /*
        Illegal Register Address:
        Set when the local APIC is in xAPIC mode and software attempts to access
        a register that is reserved in the processor's local-APIC
        register-address space; see Table 10-1. (The local-APIC register-address space
        comprises the 4 KBytes at the physical address specified in the IA32_APIC_BASE MSR.)
        Used only on Intel Core, Intel Atom, Pentium 4, Intel Xeon, and P6
        family processors. In x2APIC mode, software accesses the APIC registers
        using the RDMSR and WRMSR instructions. Use of one of these instructions
        to access a reserved register cause a general-protection exception (see Section
        10.12.1.3). They do not set the “Illegal Register Access” bit in the ESR.
    */
    klogd("\tChecking for illegal register address error... ");
    if ((value >> 7) & 0x1) {
        klogn("error\n");
        goto error;
    }
    klogn("success\n");
    return;
    error:
        kloge("INIT APIC: Failed error register check");
        halt();
}

/**
 * @brief Helper function to enable to the APIC. This function sets the
 * spurious interrupt vector to the the APIC_SPURIOUS_INT_NUM constant
 */
void apic_enable() {
    apic_write_reg(APIC_SPURIOUS_INT_VEC_REG,
                   APIC_ENABLE | APIC_SPURIOUS_INT_NUM);
    klogi("INIT APIC: APIC enabled with spurrious interrupt set to %x (%d)\n",
          APIC_SPURIOUS_INT_NUM, APIC_SPURIOUS_INT_NUM);
}

/**
 * @brief Helper function to verify if the APIC is enabled.
 * This function checks if the APIC enable bit is set in the
 * spurious interrupt vector register.
 */
static inline void apic_verify_enabled() {
    uint32_t value = static_apic_read_reg(APIC_SPURIOUS_INT_VEC_REG);

    if (value & APIC_ENABLE) {
        klogi("APIC Verification: APIC is enabled. Spurious interrupt vector set to %x (%d)\n",
              value & 0xFF, value & 0xFF);
    } else {
        kloge("APIC Verification: APIC is NOT enabled. Spurious interrupt vector register value: %x\n", value);
        halt();
    }
}

/**
 * @brief Main APIC initialization function
 */
void apic_init() {
    klogs("INIT APIC: starting...\n");

    /* Note: This method only works with CPU model families greater than 5 */
    uint32_t apic_base_msr = read_msr(IA32_APIC_BASE_MSR);

    /* Check CPU features exist */
    if (check_2xapic_exists()) {
        klogi("INIT APIC: support for x2APIC feature (IA32_APIC_BASE %4x, %s)\n",
              apic_base_msr & 0xFFFF,
              (apic_base_msr & IA32_APIC_BASE_MSR_BSP) ?
              "Bootstrap processor" :
              "Not bootstrap processor core");
    } else if (check_apic_exists()) {
        klogi("INIT APIC: support for APIC feature (IA32_APIC_BASE %4x, %s)\n",
              apic_base_msr & 0xFFFF,
              (apic_base_msr & IA32_APIC_BASE_MSR_BSP) ?
              "Bootstrap processor" :
              "Not bootstrap processor core");
    } else {
        kloge("INIT APIC: both APIC and x2APIC not supported!\n");
        halt();
    }

    /* Check if 2x APIC is supported */
    if (apic_base_msr & IA32_APIC_BASE_MSR_ENABLE) {
        two_acpi_enabled = (apic_base_msr & IA32_APIC_BASE_MSR_X2APIC);
    }

    if (two_acpi_enabled) {
        kloge("INIT APIC: x2APIC not currently supported!\n");
        halt();
    }

    local_apic_base = (void *) PHYS_TO_VIRT(madt_get_local_apic_base());

    /* Reset the error register */
    if (apic_reset_error_reg() == SYS_ERR) {
        kloge("INIT APIC: Failed to reset apic error register!\n");
        halt();
    }

    /* The APIC must be visible to all tasks */
    vm_map(NULL, (uint64_t) local_apic_base, VIRT_TO_PHYS(local_apic_base),
           1, VM_MMIO);

    klogd("INIT APIC: APIC base memory %x mapped\n", local_apic_base);

    klogd("INIT APIC: APIC VERSION %2x\n", static_apic_read_reg(APIC_LAPIC_VERSION_REG));

    klogi("INIT APIC: Enabling APIC...\n");
    apic_enable();
    apic_verify_enabled();

    /* Check to see if any errors occured */
    apic_check_error_reg();

    klogs("INIT APIC: finished...\n");
}

/* -- FUNCTIONS ASSOCIATED WITH USING THE APIC AS THE MAIN INTERRUPT TIMER -- */

/**
 * @brief Helper function to stop the timers on the APIC
 */
void apic_timer_stop() {
    uint32_t value = static_apic_read_reg(APIC_LVT_TMR_REG);
    apic_write_reg(APIC_LVT_TMR_REG, value | APIC_TIMER_MASKED);
    /* Restore the mask on the PIC so that the system timers can continue */
    pic_restore_mask();
    /* Tell the IRQ handler to not send EOI to APIC, but rather the PIC */
    apic_timer_enabled = FALSE;
}

/**
 * @brief Helper function to start the timers on the APIC
 */
static inline void apic_timer_start() {
    /* If the PIC is still open, close down all avenues */
    if (pic_get_mask() != 0xFFFF) {
        /* Save the mask for later */
        pic_save_mask();
        /* Mask all interrupts on the PIC */
        pic_disable();
    }
    uint32_t value = static_apic_read_reg(APIC_LVT_TMR_REG);
    apic_write_reg(APIC_LVT_TMR_REG, value & (~(APIC_TIMER_MASKED)));

    apic_timer_enabled = TRUE;
}

/**
 * @brief Helper for setting the frequency of the APIC system timer
 *
 * @param freq Frequency to set
 */
static inline void apic_timer_set_freq(uint64_t freq) {
    apic_write_reg(APIC_INIT_COUNT_REG, base_frequency / (freq * divisor));
}

/**
 * @brief Helper to set the mode of the APIC timer
 *
 * @param mode Either periodic or one shot mode
 */
static inline void apic_timer_set_mode(APIC_TMR_MODE mode) {
    uint32_t value = static_apic_read_reg(APIC_LVT_TMR_REG);

    if (mode == APIC_PERIODIC_MODE) {
        apic_write_reg(APIC_LVT_TMR_REG, value | APIC_TIMER_PERIODIC);
    } else {
        apic_write_reg(APIC_LVT_TMR_REG, value & (~(APIC_TIMER_PERIODIC)));
    }
}

/**
 * @brief Helper to verify if interrupt vector was set correctly
 *
 */
static inline void apic_timer_int_vector_verify() {
    if ((static_apic_read_reg(APIC_LVT_TMR_REG) & 0xFF) !=
         avail_apic_isr_vector) {
        kloge("APIC TIMER: Interrupt vector verification failed!\n");
        halt();
    }
}

/**
 * @brief Helper to start the APIC as the system timer
 */
void apic_timer_enable() {
    /* Set the timer to use divider 1 */
    apic_write_reg(APIC_DIVIDE_CONFIG_REG, 0x1);

    /* Tell the APIC to set the timer interrupt on the defined IRQ number */
    apic_write_reg(APIC_LVT_TMR_REG, APIC_TIMER_PERIODIC | avail_apic_isr_vector);

    /* Make sure the interrupt vector was set correctly */
    apic_timer_int_vector_verify();

    /* Set the APIC timer to -1 */
    apic_write_reg(APIC_INIT_COUNT_REG, UINT32_MAX);
}

/**
 * @brief Helper to disable the timer
 */
static inline void apic_timer_disable() {
    apic_write_reg(APIC_LVT_TMR_REG, APIC_TIMER_MASKED);
}

/**
 * @brief Helper function to check if the timer interrupt has been received
 */
uint8_t apic_timer_int_is_delivered() {
    return ((static_apic_read_reg(APIC_LVT_TMR_REG) >> 11) & 0xF);
}

/* Entry to context switch prototype */
void enter_ctxsw(void *v);

/* Dummy interrupt handler while figuring out timing for the bus speed */
void dummy_apic_timer_handler();

/**
 * @brief Helper to set the context switch handler to be correlated to the
 * interrupt for the APIC timer
 */
static inline void apic_timer_register_ctxsw_handler() {
    isr_register_handler(avail_apic_isr_vector, (ISR_HANDLER) enter_ctxsw);
}

/**
 * @brief Helper to set the APIC timer interrupt handler to something which
 * won't effect the system while determining bus speed
 */
static inline void apic_timer_register_dummy_handler() {
    isr_register_handler(avail_apic_isr_vector,
                         (ISR_HANDLER) dummy_apic_timer_handler);
}

/**
 * @brief Helper to read the value from the current count register
 *
 * @return uint64_t Current count
 */
uint64_t apic_timer_read_current_count() {
    return static_apic_read_reg(APIC_CURRENT_COUNT_REG);
}

/**
 * @brief Helper to determine the bus speed of the CPU for the APIC timer init
 * @note If the base frequency has already been found, then one of the APIC
 * timers has already been initialized. Thus, the PIC has been completed masked
 * and would cause the system to hang if used to determine a CPU bus speed. So,
 * use the value that was first computed for the rest of the APIC timers when
 * they're being initialized.
 */
static inline void apic_timer_speed_calc() {
    if (!base_frequency) {
        /* Register the dummy handler to use during bus speed calculation */
        apic_timer_register_dummy_handler();

        /* OSDev wiki suggests using a divisor other than 1 */
        divisor = 4;
        apic_timer_enable();

        /* Sleep for 10 PIT ticks (around 10 ms) */
        pit_sleep(10);

        /* Stop APIC timer to record number of ticks in ~10ms */
        apic_timer_disable();

        /* Now we know how often the APIC timer has ticked */
        base_frequency = ((UINT32_MAX -
                           static_apic_read_reg(APIC_CURRENT_COUNT_REG)) * 2) *
                           divisor;
    }
}

/**
 * @brief Main initialization function for the APIC to take over control of
 *        being the main system timer
 * @note These steps are the ones layed out on the OSDevWiki
 */
void apic_timer_init() {
    klogs("INIT APIC TMR: starting...\n");

    /*
        We set the APIC timer ISR vector here even though there are other checks
        elsewhere to make sure it's set because when new CPUs call this function
        we want to make sure they all have their own vector.
    */
    avail_apic_isr_vector = isr_get_avaiable_vector();

    /* Just use the speed calculated the first time, if calculated already */
    if (!base_frequency) {
        isr_register_handler(avail_apic_isr_vector,
                                (ISR_HANDLER) dummy_apic_timer_handler);

        /* OSDev wiki suggests using a divisor other than 1 */
        divisor = 3;

        /* Set the timer to use divider 1 */
        apic_write_reg(APIC_DIVIDE_CONFIG_REG, divisor);

        /* Set the APIC timer to -1 */
        apic_write_reg(APIC_INIT_COUNT_REG, UINT32_MAX);

        /* Sleep for 10 PIT ticks (around 10 ms) */
        pit_sleep(10);

        /* Stop APIC timer to record number of ticks in ~10ms */
        apic_write_reg(APIC_LVT_TMR_REG, APIC_TIMER_MASKED);

        /* Now we know how often the APIC timer has ticked */
        base_frequency = UINT32_MAX - static_apic_read_reg(APIC_CURRENT_COUNT_REG);

        /* Disable PIT from ticking since we now rely on APIC */
        pic_mask(IRQ_PIT);
    }

    /* Register enter_ctxsw */
    /*
        NOTE:
        This handler is registered directly rather than through the main general
        interrupt handler since enter_ctxsw saves registers. So, going through
        the general handler would cause the registers to be saved twice.
    */
    idt_init_entry(avail_apic_isr_vector, enter_ctxsw, GDT_KERNEL_CODE_64_BIT,
                 IDT_FLAG_RING0 | IDT_FLAG_GATE_64BIT_INT);


    /* Tell the APIC to set the timer interrupt on the defined IRQ number */
    apic_write_reg(APIC_LVT_TMR_REG, APIC_TIMER_PERIODIC | avail_apic_isr_vector);
    apic_write_reg(APIC_DIVIDE_CONFIG_REG, divisor);
    apic_write_reg(APIC_INIT_COUNT_REG, base_frequency);

    klogs("INIT APIC TMR: finished...\n");
}