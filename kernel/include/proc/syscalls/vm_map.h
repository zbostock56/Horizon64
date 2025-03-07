/**
 * @file vm_map.h
 * @author Zack Bostock
 * @brief Functionality pertaining to vm_map system call
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
uint64_t sys_vm_map(uint64_t *hint, uint64_t len, uint64_t prot,
                    uint64_t flags, uint64_t fd, uint64_t offset);

