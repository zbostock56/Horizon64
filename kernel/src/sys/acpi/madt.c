/**
 * @file madt.c
 * @author Zack Bostock
 * @brief
 * Implementation of the Multiple APIC Description Table (MADT) functionality,
 * part of the ACPI table.
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <sys/acpi/madt.h>

static MADT *madt;
static uint64_t num_local_apic = 0;
static MADT_RECORD_LAPIC *local_apics[MAX_CPUS];
static uint64_t num_io_apics = 0;
static MADT_RECORD_IO_APIC *io_apics[4];

/**
 * @brief Gets the number of IO APICS that have been found.
 *
 * @return uint32_t Number of IO APICS which have been found
 */
uint32_t madt_get_num_ioapics() {
    return num_io_apics;
}

/**
 * @brief Gets the number of local APICS that have been found.
 *
 * @return uint32_t Number of local APICS which have been found
 */
uint32_t madt_get_num_local_apics() {
    return num_local_apic;
}

/**
 * @brief
 * Returns the buffer of pointers to IO APIC structures which have been found
 *
 * @return MADT_RECORD_IO_APIC** IO APIC structures whice have been found
 */
MADT_RECORD_IO_APIC **madt_get_io_apics() {
    return io_apics;
}

/**
 * @brief
 * Returns the buffer of pointers to local APIC structures which have been found
 *
 * @return MADT_RECORD_LAPIC** Local APIC structures which have been found
 */
MADT_RECORD_LAPIC **madt_get_local_apics() {
    return local_apics;
}

/**
 * @brief Returns the base address of the local APIC in the MADT
 *
 * @return uint64_t Base address
 */
uint64_t madt_get_local_apic_base() {
    return madt->lapic_address;
}

/**
 * @brief Main MADT initialization function
 */
void madt_init() {
    klogi("INIT MADT: starting...\n");
    madt = (MADT *) acpi_get_sdt("APIC");

    if (!madt) {
        kloge("MADT INIT: \"APIC\" signature not found!\n");
        halt();
    }

    uint64_t size = madt->header.length - sizeof(MADT);
    for (uint64_t i = 0; i < size;) {
        MADT_RECORD_HEADER *record = (MADT_RECORD_HEADER *)(madt->records + i);

        /* Loop through all the entries and classify them */
        switch (record->type) {
            case MADT_RECORD_TYPE_LOCAL_APIC:
                /* Don't track if over the max CPU limit */
                if (num_local_apic >= MAX_CPUS) {
                    break;
                }
                local_apics[num_local_apic++] = (MADT_RECORD_LAPIC *) record;
                break;
            case MADT_RECORD_TYPE_IO_APIC:
                /* Don't track if over the limit */
                if (num_io_apics > MAX_IO_APICS) {
                    break;
                }
                io_apics[num_io_apics++] = (MADT_RECORD_IO_APIC *) record;
                break;
            /* TODO: Handle other types of entries */
        }
        i += record->length;
    }

    klogi("INIT MADT: finished...\n");
}
