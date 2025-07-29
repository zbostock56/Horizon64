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
 */
#define ENABLE_KLOG_DEBUG   (0)

/**
 * @brief Prints kernel logs to the framebuffer
 */
#define FRAMEBUFFER_LOGGING (0)

/**
 * @brief Denotes if memory debugging is enabled
 */
 #define KMEM_DEBUG         (0)

/**
 * @brief Denotes if the slab allocator is being used
 */
#define SLAB_ALLOCATOR      (1)

/**
 * @brief Denotes if memory allocator should use poison values or not
 */
#define SLAB_POISON         (0)
