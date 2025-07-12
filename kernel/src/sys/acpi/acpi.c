/**
 * @file acpi.c
 * @author Zack Bsotock
 * @brief
 * Implementation of Advanced Configuration and Power Interface (ACPI)
 * functions.
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <sys/acpi/acpi.h>

/* NOTE: SDT => System Descriptor Table */
static ACPI_SDT *sdt = NULL;
static uint8_t use_xsdt = FALSE;

/**
 * @brief Helper for adding together bytes in the ACPI structure.
 *
 * @param addr Starting address to add from
 * @param length Length of bytes to add
 * @return uint8_t Resulting sum
 */
uint8_t calculate_checksum(const uint8_t *addr, size_t length) {
    uint8_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += addr[i];
    }
    return sum;
}

/**
 * @brief Finds the the requested System Descriptor Table, if it exists
 *
 * @param signature The signature of the type of SDT which is being looked for
 * @return ACPI_SDT * Pointer to SDT if found, NULL otherwise.
 */
ACPI_SDT *acpi_get_sdt(const char *signature) {
    if (!signature) {
        kloge("INIT ACPI: Trying to get an SDT with a NULL signature!\n");
        return NULL;
    }

    if (!sdt) {
        kloge("INIT ACPI: SDT is NULL!\n");
        return NULL;
    }

    /* Turnary operator here is for compatibility with ver 2.0 or greater */
    size_t length = (sdt->header.length - sizeof(ACPI_SDT_HEADER)) /
                      (use_xsdt ? 8 : 4);
    for (size_t i = 0; i < length; i++) {
        ACPI_SDT *table = (ACPI_SDT *) PHYS_TO_VIRT((use_xsdt ?
                          ((uint64_t *) sdt->data)[i] :
                          ((uint32_t *) sdt->data)[i]));
        if (!memcmp(table->header.signature, signature, strlen(signature))) {
            klogd("INIT ACPI: found SDT \"%s\" in table %x\n", signature, table);
            return table;
        }
    }
    kloge("INIT ACPI: SDT \"%s\" not found\n", signature);
    return NULL;
}

/**
 * @brief Main ACPI initialization function
 *
 * @param req RSDP request from the bootloader
 */
void acpi_init(LIMINE_RSDP_REQ req) {
    klogs("INIT ACPI: starting...\n");
    LIMINE_RSDP_RES *res = req.response;

    /* Parameter check before moving on */
    if (!res) {
        kloge("INIT ACPI: Limine RSDP response is NULL!\n");
        halt();
    }

    XSDP *rsdp = (XSDP *) res->address;

    if (rsdp_checksum_check(rsdp) == SYS_ERR) {
        klogi("INIT ACPI: RSDP checksum invalid!\n");
        return;
    } else {
        klogi("INIT ACPI: RSDP checksum is valid...\n");
    }

    /* Set up the System Descriptor Table with the correct revision */
    /* based on the Root System Descriptor Pointer */
    if (rsdp->revision == 2) {
        klogi("INIT ACPI: acpi version 2.0 found...\n");
        /* Address stored in structure is the physical address */
        sdt = (ACPI_SDT *) PHYS_TO_VIRT(rsdp->xsdt_address);
        use_xsdt = TRUE;
    } else {
        klogi("INIT ACPI: acpi version 1.0 found (revision %d)...\n",
              rsdp->revision);
        /* Address stored in structure is the physical address */
        sdt = (ACPI_SDT *) PHYS_TO_VIRT(rsdp->rsdt_address);
        use_xsdt = FALSE;
    }

    /* Check RSDP and print out information about it for visual check */
    char buffer[9] = {0};
    memcpy(buffer, rsdp->signature, 8);
    klogi("INIT ACPI: RSDP signature: (%s)\n", buffer);

    /* Check to make sure the signature is correct for extra insurance */
    if (memcmp(buffer, "RSD PTR ", 8)) {
        kloge("INIT ACPI: RSDP signature is not correct! Should be 'RSD PTR '"
              " but is (%s)\n", rsdp->signature);
        halt();
    }

    memset(buffer, 0, 9);
    memcpy(buffer, rsdp->oemid, 6);
    buffer[7] = '\0';
    klogi("INIT ACPI: RSDP OEM ID: (%s)\n", buffer);

    /* Initialize Multiple APIC Description Table (MADT) */
    madt_init();
    klogs("INIT ACPI: finished...\n");
}
