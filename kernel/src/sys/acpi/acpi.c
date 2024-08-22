/**
 * @file acpi.c
 * @author Zack Bsotock
 * @brief
 * Implementation of Advanced Configuration and Power Interface (ACPI)
 * functions.
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <sys/acpi/acpi.h>

/* NOTE: SDT => System Descriptor Table */
static ACPI_SDT *sdt = NULL;
static uint8_t use_xsdt = FALSE;

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
    size_t i = 0;
    for (; i < length; i++) {
        ACPI_SDT *table = (ACPI_SDT *) PHYS_TO_VIRT((use_xsdt ?
                          ((uint64_t *) sdt->data)[i] :
                          ((uint32_t *) sdt->data)[i]));
        if (!memcmp(table->header.signature, signature, strlen(signature))) {
            klogi("INIT ACPI: found SDT \"%s\" %x\n", signature, table);
            return table;
        }
    }
    klogi("INIT ACPI: SDT \"%s\" not found\n", signature);
    return NULL;
}

/**
 * @brief Main ACPI initialization function
 *
 * @param req RSDP request from the bootloader
 */
void acpi_init(LIMINE_RSDP_REQ req) {
    klogi("INIT ACPI: starting...\n");
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
    klogi("INIT ACPI: RSDP signature: (");
    for (uint8_t i = 0; i < 8; i++) {
        klogi("%c", rsdp->signature[i]);
    }
    klogi(")\n");

    /* Check to make sure the signature is correct for extra insurance */
    if (memcmp(rsdp->signature, "RSD PTR ", 8)) {
        kloge("INIT ACPI: RSDP signature is not correct! Should be 'RSD PTR '.");
        return;
    }

    klogi("INIT ACPI: RSDP OEM ID: (");
    for (uint8_t i = 0; i < 6; i++) {
        klogi("%c", rsdp->signature[i]);
    }
    klogi(")\n");

    /* Initialize Multiple APIC Description Table (MADT) */
    madt_init();
    klogi("INIT ACPI: finished...\n");
}
