/**
 * @file gpf_decode.c
 * @author Zack Bostock
 * @brief Functionality pertaining to decoding General Protection Fault error
 *        codes
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <util/gpf_decode.h>

#include <structs/gdt_str.h>

#include <common/kprint.h>

/**
 * @brief Decodes the error code passed from the interrupt handler to make
 *        it easier to read in the traceback
 * 
 * @param error_code Error code from interrupt handler
 */
void gpf_decode(uint64_t error_code) {
    /* When error code is 0, decoding error code has no meaning */
    if (error_code == 0) {
        return;
    }

    klog_toggle_print_prefix(FALSE);
    int external = (0x1 & error_code);
    int table = (0x3 & (error_code >> 1));
    int index = (0x1FFF & (error_code >> 3));
    klogn(" 31         16   15             3   2   1   0\n");
    klogn("+---+--  --+---+---+--      --+---+---+---+---+\n");
    klogn("|   Reserved   | %13b | %1b   %1b | %1b |\n",
            index,
            table & 0x2,
            table & 0x1,
            external & 0x1);
    klogn("+---+--  --+---+---+--      --+---+---+---+---+\n\n");

    /* Print information about what each set bit means */
    if (external) {
        klogn("External Processor Exception: true\n");
    } else {
        klogn("External Processor Exception: false\n");
    }

    if ((table & 0x3) == 0) {
        klogn("Table Bit: Descriptor in GDT\n");
    } else if ((table & 0x3) == 1 || ((table & 0x3) == 3)) {
        klogn("Table Bit: Descriptor in IDT\n");
    } else {
        klogn("Table Bit: Descriptor in LDT\n");
    }

    if ((table & 0x3) == 0) {
        klogn("Selector Index: 0x%2x (%d)\n", index, index);
        klogn("GDT Segment: %s\n\n", gdt_member_names[index]);
    } else {
        klogn("Selector Index: 0x%2x (%d)\n\n", index, index);
    }

    klog_toggle_print_prefix(TRUE);
}