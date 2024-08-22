/**
 * @file rsdp.c
 * @author Zack Bostock
 * @brief Functionality associated with the Root System Descriptor Table (RSDP)
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <sys/acpi/rsdp.h>

/**
 * @brief Helper for adding together bytes in the RSDP structure.
 *
 * @param addr Starting address to add from
 * @param length Length of bytes to add
 * @return uint8_t Resulting sum
 */
static uint8_t calculate_checksum(const uint8_t *addr, size_t length) {
    uint8_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += addr[i];
    }
    return sum;
}

/**
 * @brief Checks the checksum of the RSDP to ensure the data is valid
 * @verbatim
 * Before the RSDP is relied upon you should check that the checksum is valid.
 * For ACPI 1.0, you add up every byte in the structure and make sure that
 * the lowest byte of the result is equal to zero. For ACPI 2.0 and later, you'd
 * do exactly the same thing as ACPI 1.0, but also include the other parts of
 * the structure which encompass the ACPI 2.0 extension.
 *
 * @param XSDP RSDP to check
 * @return STATUS SYS_OK if good, SYS_ERR if bad checksum
 */
STATUS rsdp_checksum_check(XSDP *rsdp) {
    /* Check the first RSDP structure, if this is already bad just return err */
    if (calculate_checksum((const uint8_t *) rsdp, sizeof(RSDP)) != 0) {
        return SYS_ERR;
    }

    /* If it is a higher revision, check the rest */
    if (rsdp->revision >= 2) {
        if (calculate_checksum((const uint8_t *) rsdp, rsdp->length) != 0) {
            return SYS_ERR;
        }
    }
    return SYS_OK;
}
