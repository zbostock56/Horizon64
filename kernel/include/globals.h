/**
 * @file globals.h
 * @author Zack Bostock
 * @brief Global scope initialization area for global variables
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <const.h>
#include <limine.h>
#include <structs/psf_structs.h>

/* Used to as a status message response after function completion */
typedef uint8_t STATUS;

/* PSF1 FONT */
extern PSF1_FONT font;