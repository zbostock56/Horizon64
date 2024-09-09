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
 * @copyright Copyright (c) 2024
 * 
 */

#include <sys/acpi/apic.h>
#include <sys/tick/pic.h>
#include <sys/tick/clkhandler.h>
#include <sys/interrupts/irq.h>

/* Globals related to the APIC initialization */
static int two_acpi_enabled = 0;
volatile void *local_apic_base = NULL;

/* Globals related to APIC as the system timer */
static uint8_t apic_timer_enabled = 0;
static uint64_t base_frequency = 0;
static uint8_t divisor = 0;

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
inline void apic_write_reg(uint16_t offset, uint32_t value) {
    if (!local_apic_base) {
        return;
    }
    *(uint32_t volatile *)(local_apic_base + offset) = value;
}

/**
 * @brief Helper for reading an APIC register
 * 
 * @param offset Register to read from
 * @return uint32_t Value from the register 
 */
inline uint32_t apic_read_reg(uint16_t offset) {
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
    if (apic_timer_enabled) {
        apic_write_reg(APIC_EOI_REG, 0);
    }
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
    return !((apic_read_reg(APIC_ERROR_STATUS_REG) >> 1) & 0x1);
}

/**
 * @brief Helper for reading values from the error register
 */
void apic_check_error_reg() {
    klogi("APIC ERROR CHECK:\n");
    uint32_t value = apic_read_reg(APIC_ERROR_STATUS_REG);
    /*
        Send Checksum Error:
        Set when the local APIC detects a checksum error for a message that
        it sent on the APIC bus. Used only on P6 family and Pentium processors.
    */
    klogi("\tChecking for send checksum error... ");
    if ((value >> 0) & 0x1) {
        klogi("error\n");
        goto error;
    }
    klogi("success\n");

    /*
        Receive Checksum Error:
        Set when the local APIC detects a checksum error for a message that
        it received on the APIC bus. Used only on P6 family and Pentium processors.
    */
    klogi("\tChecking for receive checksum error... ");
    if ((value >> 1) & 0x1) {
        klogi("error\n");
        goto error;
    }
    klogi("success\n");

    /*
        Send Accept Error:
        Set when the local APIC detects that a message it sent was not accepted
        by any APIC on the APIC bus. Used only on P6 family and Pentium processors.
    */
    klogi("\tChecking for send accept error... ");
    if ((value >> 2) & 0x1) {
        klogi("error\n");
        goto error;
    }
    klogi("success\n");

    /*
        Receive Accept Error:
        Set when the local APIC detects that a message it received was not accepted
        by any APIC on the APIC bus. Used only on P6 family and Pentium processors.
    */
    klogi("\tChecking for receive accept error... ");
    if ((value >> 3) & 0x1) {
        klogi("error\n");
        goto error;
    }
    klogi("success\n");

    /*
        Redirectable IPI Error:
        Set when the local APIC detects an attempt to send an IPI with the
        lowest-priority delivery mode and the local APIC does not support the
        sending of such IPIs. This bit is used on some Intel Core and Intel
        Xeon processors.
    */
    klogi("\tChecking for redirectable IPI error... ");
    if ((value >> 4) & 0x1) {
        klogi("error\n");
        goto error;
    }
    klogi("success\n");

    /*
        Send Illegal Vector Error:
        Set when the local APIC detects an illegal vector (one in the range 0
        to 15) in the message that it is sending. This occurs as the result of
        a write to the ICR (in both xAPIC and x2APIC modes) or to SELF IPI register
        (x2APIC mode only) with an illegal vector
    */
    klogi("\tChecking for send illegal vector error... ");
    if ((value >> 5) & 0x1) {
        klogi("error\n");
        goto error;
    }
    klogi("success\n");


    /*
        Receive Illegal Vector Error:
        Set when the local APIC detects an illegal vector (one in the range 0
        to 15) in an interrupt message it receives or in an interrupt generated
        locally from the local vector table or via a self IPI. Such interrupts are not delivered
        to the processor; the local APIC will never set an IRR bit in the range 0 to 15.
    */
    klogi("\tChecking for send illegal vector error... ");
    if ((value >> 6) & 0x1) {
        klogi("error\n");
        goto error;
    }
    klogi("success\n");

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
    klogi("\tChecking for illegal register address error... ");
    if ((value >> 7) & 0x1) {
        klogi("error\n");
        goto error;
    }
    klogi("success\n");
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
 * @brief Main APIC initialization function
 */
void apic_init() {
    klogi("INIT APIC: starting...\n");
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
    apic_reset_error_reg();

    /* The APIC must be visible to all tasks */
    vm_map(NULL, (uint64_t) local_apic_base, VIRT_TO_PHYS(local_apic_base),
           1, VM_MMIO);
    
    klogi("INIT APIC: APIC base memory %x mapped\n", local_apic_base);

    klogi("INIT APIC: Enabling APIC...\n");
    apic_enable();

    klogi("INIT APIC: APIC VERSION %2x\n", apic_read_reg(APIC_LAPIC_VERSION_REG));

    /* Check to see if any errors occured */
    apic_check_error_reg();

    klogi("INIT APIC: finished...\n");
}

/* -- FUNCTIONS ASSOCIATED WITH USING THE APIC AS THE MAIN INTERRUPT TIMER -- */

/**
 * @brief Helper function to stop the timers on the APIC
 */
void apic_timer_stop() {
    uint32_t value = apic_read_reg(APIC_LVT_TMR_REG);
    apic_write_reg(APIC_LVT_TMR_REG, value | APIC_TIMER_MASKED);
    /* Restore the mask on the PIC so that the system timers can continue */
    pic_restore_mask();
    /* Tell the IRQ handler to not send EOI to APIC, but rather the PIC */
    apic_timer_enabled = 0;
}

void clkhandler_two(REGISTERS *);

/**
 * @brief Helper function to start the timers on the APIC
 */
void apic_timer_start() {
    /* Tell the IRQ handler to not send EOI to PIC, but rather the APIC */
    if (pic_get_mask() != 0xFFFF) {
        /* Save the mask for later */
        pic_save_mask();
        /* TODO: Should mask all interrupts on the PIC, but this would stop */
        /*       the use of the keyboard. So, just mask IRQ 0 where the     */
        /*       system timer was initially set.                            */
        /* Mask all interrupts on the PIC */
        //pic_disable();
        pic_mask(0);
    }
    irq_register_handler(0, clkhandler_two);
    uint32_t value = apic_read_reg(APIC_LVT_TMR_REG);
    apic_write_reg(APIC_LVT_TMR_REG, value & (~(APIC_TIMER_MASKED)));

    apic_timer_enabled = 1;
}

/**
 * @brief Helper for setting the frequency of the APIC system timer
 * 
 * @param freq Frequency to set
 */
void apic_timer_set_freq(uint64_t freq) {
    apic_write_reg(APIC_INIT_COUNT_REG, base_frequency / (freq * divisor));
}

/**
 * @brief Helper to set the mode of the APIC timer
 * 
 * @param mode Either periodic or one shot mode
 */
void apic_timer_set_mode(APIC_TMR_MODE mode) {
    uint32_t value = apic_read_reg(APIC_LVT_TMR_REG);

    if (mode == APIC_PERIODIC_MODE) {
        apic_write_reg(APIC_LVT_TMR_REG, value | APIC_TIMER_PERIODIC);
    } else {
        apic_write_reg(APIC_LVT_TMR_REG, value & (~(APIC_TIMER_PERIODIC)));
    }
}

/**
 * @brief Helper to start the APIC as the system timer
 */
void apic_timer_enable() {
    /* Tell the APIC to set the timer interrupt on the defined IRQ number */
    apic_write_reg(APIC_LVT_TMR_REG, APIC_TIMER_MASKED | APIC_HW_INT_NUM);
    /* Set the timer to use divider 1 */
    apic_write_reg(APIC_DIVIDE_CONFIG_REG, 0x1);
    /* Set the APIC timer to -1 */
    apic_write_reg(APIC_INIT_COUNT_REG, UINT32_MAX);
}

/**
 * @brief Helper function to check if the timer interrupt has been received
 */
uint8_t apic_timer_int_is_delivered() {
    return ((apic_read_reg(APIC_LVT_TMR_REG) >> 11) & 0xF);
}


/**
 * @brief Main initialization function for the APIC to take over control of
 *        being the main system timer
 */
void apic_timer_init() {
    klogi("INIT APIC TMR: starting...\n");

    apic_reset_error_reg();

    /* OSDev wiki suggests using a divisor other than 1 */
    divisor = 4;
    apic_timer_enable();

    /* Sleep for 1 PIT tick (around 1 ms) */
    system_timer_sleep(1);
    
    /* Now we know how often the APIC timer has ticked */
    base_frequency = ((UINT32_MAX - apic_read_reg(APIC_CURRENT_COUNT_REG)) *
                     2) * divisor;

    /* Set the APIC to the frequency we want based on 1 ms */
    apic_timer_set_freq(base_frequency);
    /* Set periodic mode to act as system timer */
    apic_timer_set_mode(APIC_PERIODIC_MODE);
    /* Start timer (disables PIT as main system timer) */
    apic_timer_start();

    klogi("INIT APIC TMR: Base frequency: %d Hz | Divisor: %d | IRQ %d\n",
            base_frequency, divisor, APIC_HW_INT_NUM - PIC_REMAP_OFFSET);

    apic_check_error_reg();

    klogi("INIT APIC TMR: finished...\n");
}