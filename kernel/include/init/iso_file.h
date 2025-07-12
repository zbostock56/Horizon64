/**
 * @file iso_file.h
 * @author Zack Bostock
 * @brief Information pertaining to using files from the system image
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <globals.h>

#include <common/limine_typedefs.h>

#include <sys/asm.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void get_iso_file(const char *name, LIMINE_MODULE_REQ module_request,
                  LIMINE_FILE **file);
STATUS check_string_ending(const char *str, const char *end);
