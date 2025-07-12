/**
 * @file pf_decode.c
 * @author Zack Bostock
 * @brief Functionality to decode #PF error code
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <util/pf_decode.h>

#include <common/kprint.h>

/**
 * @brief Decodes the error code passed from the interrupt handler to make
 *        it easier to read in the traceback
 * 
 * @param error_code Error code from interrupt handler
 */
void pf_decode(uint64_t error_code, uint64_t cr2) {
    klog_toggle_print_prefix(FALSE);

    int present = (0x1 & error_code);
    int write = (0x2 & error_code);
    int user = (0x4 & error_code);
    int reserved_write = (0x8 & error_code);
    int instruction_fetch = (0x10 & error_code);
    int protection_key = (0x20 & error_code);
    int shadow_stack = (0x40 & error_code);
    int software_guard_extensions = (0x4000 & error_code);

    klogn(" 31              15                             4               0\n");
    klogn("+---+--  --+---+-----+---+--  --+---+----+----+---+---+---+---+---+\n");
    klogn("|   Reserved   | %1b |   Reserved   |  %1b  |  %1b  | %1b | %1b | %1b | %1b | %1b |\n",
            present, write, user, reserved_write, instruction_fetch,
            protection_key, shadow_stack, software_guard_extensions);
    klogn("+---+--  --+---+-----+---+--  --+---+----+----+---+---+---+---+---+\n\n");

    klogn("Virtual Address (CR2): 0x%x\n", cr2);
    
    if (present) {
        klogn("Page Protection Violation: true\n");
    } else {
        klogn("Page Protection Violation: false\n");
    }

    klogn("Caused by: Write Access | Read Access\n");
    if (write) {
        klogn("           ^^^^^^^^^^^^\n");
    } else {
        klogn("                          ^^^^^^^^^^^\n");
    }

    if (reserved_write) {
        klogn("Caused while Current Privilege Level (CPL) = 3: true\n");
    } else {
        klogn("Caused while Current Privilege Level (CPL) = 3: false\n");
    }

    if (instruction_fetch) {
        klogn("Caused by instruction fetch: true\n");
    } else {
        klogn("Caused by instruction fetch: false\n");
    }

    if (protection_key) {
        klogn("Caused by protection-key violation: true\n");
    } else {
        klogn("Caused by protection-key violation: false\n");
    }

    if (shadow_stack) {
        klogn("Caused by shadow stack access: true\n");
    } else {
        klogn("Caused by shadow stack access: false\n");
    }

    if (software_guard_extensions) {
        klogn("Caused by SGX violation: true\n\n");
    } else {
        klogn("Caused by SGX violation: false\n\n");
    }

    klog_toggle_print_prefix(TRUE);
}