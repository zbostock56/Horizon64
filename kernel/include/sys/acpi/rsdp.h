/**
 * @file rsdp.h
 * @author Zack Bostock
 * @brief Functions associated with the Root System Descriptor Table (RSDP)
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>

#include <globals.h>

#include <structs/rsdp_str.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
STATUS rsdp_checksum_check(XSDP *rsdp);
