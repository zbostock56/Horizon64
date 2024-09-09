/**
 * @file clkhandler.h
 * @author Zack Bostock
 * @brief Information pertaining to the system timer handler
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>
#include <structs/regs_str.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
uint64_t get_pit_time();
void system_timer_sleep(uint64_t offset);
void clkhandler(REGISTERS *reg);