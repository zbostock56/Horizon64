/**
 * @file initrd.h
 * @author Zack Bostock
 * @brief Information for loading the initrd
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <init/iso_file.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void initrd_init(struct limine_module_request req);