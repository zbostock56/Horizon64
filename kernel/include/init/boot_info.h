/**
 * @file boot_info.h
 * @author Zack Bostock
 * @brief Information pertaining to info about the bootloader
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <common/limine_typedefs.h>
#include <common/kprint.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
STATUS print_boot_info(LIMINE_BL_INFO_REQ req);
