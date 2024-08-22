/**
 * @file boot_info.c
 * @author Zack Bostock
 * @brief Functionality pertaining to getting info about the bootloader
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <init/boot_info.h>

/**
 * @brief Helper function to print out information about the bootloader
 *
 * @param req Request for bootloader info from Limine
 * @return STATUS SYS_OK if no error, SYS_ERR otherwise
 */
STATUS print_boot_info(LIMINE_BL_INFO_REQ req) {
    LIMINE_BL_INFO_RES *res = req.response;
    if (!res) {
        kloge("INIT BL INFO: request for bootloader info is NULL!\n");
        return SYS_ERR;
    }
    klogi("INIT BL INFO:\nName: %s\nVersion: %s\n",
          res->name, res->version);
    return SYS_OK;
}
