/**
 * @file iso_file.c
 * @author Zack Bostock
 * @brief Helpers for reading from files in the system image
 * @verbatim
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <init/iso_file.h>
#include <common/kprint.h>

/**
 * @brief Helper to see if the string ends which a specific string sequence.
 *
 * @param str String to search through
 * @param end Ending to look for
 * @return STATUS SYS_OK if the specified ending, SYS_ERR otherwise
 */
STATUS check_string_ending(const char *str, const char *end) {
    const char *_str = str;
    const char *_end = end;

    while (*str != 0) {
        str++;
    }
    str--;

    while (*end != 0) {
        end++;
    }
    end--;

    while (1) {
        if (*str != *end) {
            return 0;
        }

        str--;
        end--;

        if (end == _end || (str == _str && end == _end)) {
            return SYS_OK;
        }

        if (str == _str) {
            return SYS_ERR;
        }
    }

    return SYS_ERR;
}

/*
    Expects the file name to be passed in with the limine module request
    associated with that file
*/

/**
 * @brief Get the iso file object from the bootloader
 * @verbatim
 * Expects the file name to be passed in with the limine module request
 * associated with that file
 *
 * @param name Name of the file
 * @param module_request Module from the bootloader
 * @param file File to return through
 */
void get_iso_file(const char *name, LIMINE_MODULE_REQ module_request,
                  LIMINE_FILE **file) {
    LIMINE_MODULE_RESP *module_response = module_request.response;

    /* Check bootloader provided return value */
    if (!module_response) {
        kloge("GET ISO FILE: request's response is NULL!\n");
        halt();
    }

    /* Loop through modules and look for one that matches */
    for (size_t i = 0; i < module_response->module_count; i++) {
        LIMINE_FILE *f = module_response->modules[i];
        if (check_string_ending(f->path, name)) {
            *file = f;
            return;
        }
    }
    *file = NULL;
}
