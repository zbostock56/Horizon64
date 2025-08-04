/**
 * @file ttyfs_str.h
 * @author Zack Bostock
 * @brief Structures pertaining to TTY filesystem
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <structs/vfs_str.h>

/**
 * @brief TTY buffer configuration
 */
#define TTY_BUFFER_SIZE         4096        /* Size of input/output buffers */
#define TTY_MAX_DEVICES         256         /* Maximum number of TTY devices */
#define TTY_CANONICAL_MAX_LINE  4095        /* Maximum canonical line length */
#define TTY_RAW_MIN_READ        1           /* Minimum read size in raw mode */
#define TTY_TIMEOUT_DEFAULT     0           /* Default timeout (no timeout) */

/**
 * @brief TTY device types
 */
typedef enum {
    TTY_TYPE_CONSOLE = 0,                   /* System console */
    TTY_TYPE_SERIAL,                        /* Serial port */
    TTY_TYPE_PSEUDO,                        /* Pseudo-terminal */
    TTY_TYPE_VIRTUAL,                       /* Virtual terminal */
    TTY_TYPE_NETWORK,                       /* Network terminal */
    TTY_TYPE_COUNT                          /* Number of TTY types */
} TTY_TYPE;

/**
 * @brief TTY device states
 */
typedef enum {
    TTY_STATE_CLOSED = 0,                   /* Device is closed */
    TTY_STATE_OPENING,                      /* Device is being opened */
    TTY_STATE_OPEN,                         /* Device is open and ready */
    TTY_STATE_CLOSING,                      /* Device is being closed */
    TTY_STATE_ERROR,                        /* Device is in error state */
    TTY_STATE_SUSPENDED                     /* Device is suspended */
} TTY_STATE;

/**
 * @brief TTY buffer state flags
 */
#define TTY_BUFFER_OVERFLOW     0x01        /* Buffer overflow occurred */
#define TTY_BUFFER_UNDERFLOW    0x02        /* Buffer underflow occurred */
#define TTY_BUFFER_LOCKED       0x04        /* Buffer is locked */
#define TTY_BUFFER_DIRTY        0x08        /* Buffer contains unsaved data */

/**
 * @brief TTY input processing flags
 */
#define TTY_INPUT_RAW           0x01        /* Raw input mode */
#define TTY_INPUT_CANONICAL     0x02        /* Canonical input mode */
#define TTY_INPUT_ECHO_OFF      0x04        /* Echo is disabled */
#define TTY_INPUT_SIGNAL_PROC   0x08        /* Signal processing enabled */

/**
 * @brief TTY identifier structure
 * This is the main structure that represents a TTY device instance
 */
typedef struct {
    char ibuff[TTY_BUFFER_SIZE];            /* Input buffer */
    int64_t ibegin;                         /* Input buffer start */
    int64_t icursor;                        /* Input buffer cursor */
    int64_t isize;                          /* Input buffer size */
    volatile bool line_ready;               /* Complete line available for read */

} TTYFS_IDENT;

/**
 * @brief Error codes specific to TTY operations
 */
#define TTY_SUCCESS             0           /* Operation successful */
#define TTY_ERROR_INVALID_ARG   -1          /* Invalid argument */
#define TTY_ERROR_NO_MEMORY     -2          /* Out of memory */
#define TTY_ERROR_DEVICE_BUSY   -3          /* Device is busy */
#define TTY_ERROR_NOT_FOUND     -4          /* Device not found */
#define TTY_ERROR_ACCESS_DENIED -5          /* Access denied */
#define TTY_ERROR_TIMEOUT       -6          /* Operation timed out */
#define TTY_ERROR_OVERFLOW      -7          /* Buffer overflow */
#define TTY_ERROR_UNDERFLOW     -8          /* Buffer underflow */
#define TTY_ERROR_HARDWARE      -9          /* Hardware error */
#define TTY_ERROR_NOT_SUPPORTED -10         /* Operation not supported */
#define TTY_ERROR_INTERRUPTED   -11         /* Operation interrupted */
#define TTY_ERROR_WOULD_BLOCK   -12         /* Operation would block */