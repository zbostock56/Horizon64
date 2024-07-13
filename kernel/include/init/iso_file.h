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

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* ----------------------------- STATIC GLOBALS ----------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
int check_string_ending(const char *str, const char *end);
void get_iso_file(const char *name, LIMINE_MODULE_REQ module_request,
                  LIMINE_FILE **file);
/* --------------------------- EXTERNALLY DEFINED --------------------------- */