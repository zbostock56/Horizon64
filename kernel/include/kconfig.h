/**
 * @file kconfig.h
 * @author Zack Bostock
 * @brief Holds information pertaining to the configuration of the kernel
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stddef.h>

/**
 * @brief True if command line interface is enabled
 */
#define CLI                 (1)

/**
 * @brief True if only using the bootstrap processor
 */
#define BSP_ONLY            (1)

/**
 * @brief Enables the printing of debug level logs
 *
 */
#define ENABLE_KLOG_DEBUG   (0)

/**
 * @brief Prints kernel logs to the framebuffer
 *
 */
#define FRAMEBUFFER_LOGGING (0)
