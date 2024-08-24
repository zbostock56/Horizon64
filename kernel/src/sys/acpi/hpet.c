/**
 * @file hpet.c
 * @author Zack Botock
 * @brief Functionality pertaining to the High Precision Event Timer (HPET)
 * @verbatim
 * HPET, or High Precision Event Timer, is a piece of hardware designed by
 * Intel and Microsoft to replace older PIT and RTC. It consists of
 * (usually 64-bit) main counter (which counts up), as well as from 3
 * to 32 32-bit or 64-bit wide comparators. HPET is programmed using memory
 * mapped IO, and the base address of HPET can be found using ACPI.
 * @ref https://wiki.osdev.org/HPET
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <sys/acpi/hpet.h>

HPET *hpet = NULL;
static HPET_SDT *hpet_sdt = NULL;
/* NOTE: The number which is stored in the data structure is one less than */
/*       the number which is stored here */
static uint16_t num_hpet_comparators = HPET_MAX_COMPARATOR;
static uint64_t hpet_period = 0;

/**
 * @brief Helper function which unsets the ENABLE_CNF value in the
 *        general configuration register
 */
inline void hpet_halt() {
    GEN_CONFIG_UNSET(ENABLE_CNF, hpet)
}

/**
 * @brief Helper function which sets the ENABLE_CNF value in the
 *        general configuration register
 */
inline void hpet_enable() {
    GEN_CONFIG_SET(ENABLE_CNF, hpet)
}

/**
 * @brief Helper for checking if HPET is capable of 64-bit operation or not
 * 
 * @return uint8_t LONG_MODE_OPERATION if usable, all other values assume fail 
 */
static inline uint8_t hpet_operating_mode() {
    if (!hpet) {
        return NO_HPET_INITIALIZED;
    }
    return GEN_CAPABILITIES_READ(COUNT_SIZE_CAP, hpet);
}

/**
 * @brief Helper for finding out the value which the main counter is storing
 *        in the HPET
 * 
 * @return uint64_t Value of the main counter
 */
static inline uint64_t hpet_read_main_counter_value() {
    if (!hpet) {
        return NO_HPET_INITIALIZED;
    }
    if (hpet_operating_mode() != LONG_MODE_OPERATION) {
        return NO_HPET_INITIALIZED;
    }
    return hpet->main_counter_value;
}

/**
 * @brief Helper for reading the revision number of the HPET
 * 
 * @return uint8_t Revision number
 */
static inline uint8_t hpet_read_revision_number() {
    if (!hpet) {
        return NO_HPET_INITIALIZED;
    }
    return (hpet->general_capabilities & 0xFF);
}

/**
 * @brief Helper for finding the number of hpet counters
 * @note What is returned from HPET is num_timers - 1
 */
static inline void hpet_find_number_timers() {
    if (!hpet) {
        return;
    }
    num_hpet_comparators = ((hpet->general_capabilities >> 8) & 0xF);
}

/**
 * @brief Helper function for resetting the main counter value
 * @note This function does not restart the HPET. That is the onus
 *       of the caller.
 * 
 * @return STATUS SYS_OK if success, SYS_ERR otherwise
 */
static inline STATUS hpet_reset_main_counter_value() {
    if (!hpet) {
        return SYS_ERR;
    }
    /* HPET must be disabled before writing to the main counter value */
    hpet_halt();
    hpet->main_counter_value = 0;
    if (hpet_read_main_counter_value() != 0) {
        return SYS_ERR;
    }
    return SYS_OK;
}

/**
 * @brief Helper for resetting a timer in the HPET
 * 
 * @param timer Timer to reset
 * @return STATUS SYS_OK if success, SYS_ERR otherwise
 */
static inline STATUS hpet_reset_comparators() {
    if (!hpet) {
        return SYS_ERR;
    }

    for (uint16_t i = 0; i < num_hpet_comparators; i++) {
        klogi("INIT HPET: Timer %d's Capabilities:\n", i + 1);
        HPET_TIMER *timer = (hpet->timers) + i;
        /* Check if the timer supports FSB routing */
        if (HPET_TIMER_READ_CONFIG_CAP(TMR_FSB_INT_DEL_CAP, timer)) {
            klogi("\tSupports FSB routing...\n");
        } else {
            klogi("\tDoes not support FSB routing...\n");
        }
        
        /* Check if the timer will use FSB interrupt mapping */
        if (HPET_TIMER_READ_CONFIG_CAP(TMR_FSB_EN_CNF, timer)) {
            klogi("\tFSB interrupt mapping is enabled, disabling...\n");
            timer->config_and_capabilities &= (0 << TMR_FSB_EN_CNF);
        } else {
            klogi("\tFSB intterupt mapping is disabled...\n");
        }

        /* Check to see if the timer is 64-bit capable */
        if (HPET_TIMER_READ_CONFIG_CAP(TMR_SIZE_CAP, timer)) {
            klogi("\t64-bit mode operation...\n");
        } else {
            klogi("\t32-bit mode operation...\n");
        }

        /* Check to see if the timer supports periodic mode */
        if (HPET_TIMER_READ_CONFIG_CAP(TMR_PER_INT_CAP, timer)) {
            klogi("\tSupports periodic mode...\n");
        } else {
            klogi("\tDoes not support periodic mode...\n");
        }

        /* Check to see if interrupts are enabled, disable if so */
        if (HPET_TIMER_READ_CONFIG_CAP(TMR_INT_ENB_CNF, timer)) {
            klogi("\tInterrupts are enabled, disabling...\n");
            timer->config_and_capabilities &= (0 << TMR_INT_ENB_CNF);
        } else {
            klogi("\tInterrupts are disabled...\n");
        }

        /* Check what type of interrupts are used for this timer */
        if (HPET_TIMER_READ_CONFIG_CAP(TMR_INT_TYPE_CNF, timer)) {
            klogi("\tUses level triggered interrupts...\n");
        } else {
            klogi("\tUses edge triggered interrupts...\n");
        }
    }
    return SYS_OK;
}

/**
 * @brief Main initialization function for the HPET
 * @verbatim
 * General initialization:
 * 1. Find HPET base address in 'HPET' ACPI table.
 * 2. Calculate HPET frequency (f = 10^15 / period).
 * 3. Save minimal tick (either from ACPI table or configuration register).
 * 4. Initialize comparators.
 * 5. Set ENABLE_CNF bit.
 * 
 * Timer N initialization:
 * 1. Determine if timer N is periodic capable, save that information to avoid
 * re-reading it every time.
 * 2. Determine allowed interrupt routing for current timer and allocate an
 * interrupt for it.
 * 
 * @return STATUS SYS_OK if success, SYS_ERR otherwise
 */
STATUS hpet_init() {
    klogi("INIT HPET: starting...\n");

    /* Find HPET System Descriptor in SDT */
    hpet_sdt = (HPET_SDT *) acpi_get_sdt("HPET");
    if (!hpet_sdt) {
        kloge("INIT HPET: HPET not found in ACPI table!\n");
        hpet_sdt = NULL;
        return SYS_ERR;
    }

    /* Map as memory mapped IO device, should be visible to all               */
    vm_map(NULL, PHYS_TO_VIRT(hpet_sdt->address.address),
           (uint64_t) hpet_sdt->address.address, 1, VM_MMIO);

    hpet = (HPET *) PHYS_TO_VIRT(hpet_sdt->address.address);

    /* Read the revision number and print it out                              */
    if (hpet_sdt->hw_rev_id == 0) {
        kloge("INIT HPET: HPET revision number (%d) is too low!\n",
               hpet_sdt->hw_rev_id);
    }
    klogi("INIT HPET: HPET hardware rev. %d\n", hpet_sdt->hw_rev_id);

    /* Check to see what the routing of the IRQ's are for the HPET.           */
    /* Refer to OSDev Wiki for mappings.                                      */
    if (!(GEN_CAPABILITIES_READ(LEG_RT_CAP, hpet))) {
        kloge("INIT HPET: HPET is not legacy replacement capable!\n");
        hpet = NULL;
        hpet_sdt = NULL;
        return SYS_ERR;
    }

    /* Check to see how many HPET comparators there are and print them        */
    num_hpet_comparators = hpet_sdt->comparator_count + 1;
    if (HPET_NUM_COMPARATOR_CHECK(num_hpet_comparators)) {
        kloge("INIT HPET: Number of comparators (%d) is out of valid range!\n",
              num_hpet_comparators);
        hpet = NULL;
        hpet_sdt = NULL;
        return SYS_ERR;
    }
    klogi("INIT HPET: %d comparators total\n", num_hpet_comparators);
    
    /* Reset the timers to a known state */
    hpet_reset_comparators();

    #if HPET_LONG_MODE 
    /* Check to see if the HPET can operate in 64-bit mode. If not, don't use */
    /* since 32-bit operation should only be used in Protected Mode.          */
    if (hpet_operating_mode() != LONG_MODE_OPERATION) {
        kloge("INIT HPET: HPET is not able to operate in 64-bit mode!\n");
        hpet = NULL;
        hpet_sdt = NULL;
        return SYS_ERR;
    }
    #endif

    /* Calculate HPET frequency where freq = 10^15 / period                   */
    uint64_t counter_clk_period = hpet->general_capabilities >> 32;
    uint64_t freq = 1000000000000000 / counter_clk_period;

    klogi("INIT HPET: HPET frequency detected as %d HZ\n", freq);
    hpet_period = counter_clk_period / 1000000;

    #if HPET_LONE_MODE
    klogi("INIT HPET: Current value of main counter: %d\n",
          hpet_read_main_counter_value());

    klogi("INIT HPET: Resetting main counter value...\n");
    if (hpet_reset_main_counter_value() != SYS_OK) {
        kloge("INIT HPET: Failed to reset main counter value!\n");
    }
    #endif

    /* Set the ENABLE_CNF bit                                                 */
    hpet_enable();

    klogi("INIT HPET: finished...\n");
    return SYS_OK;
}
