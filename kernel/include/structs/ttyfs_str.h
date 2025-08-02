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
#include <structs/termios_str.h>

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
 * @brief Basic TTY file structure
 */
typedef struct {
    char name[VFS_MAX_NAME_LEN];            /* TTY device name */
    TTY_TYPE type;                          /* Device type */
    uint32_t device_id;                     /* Unique device identifier */
    uint32_t flags;                         /* Device-specific flags */
} TTYFS_FILE;

/**
 * @brief TTY directory entry structure
 */
typedef struct {
    TTYFS_FILE entry;                       /* Basic file information */
    STD_TIME create_time;                   /* Creation time */
    STD_TIME access_time;                   /* Last access time */
    STD_TIME modify_time;                   /* Last modification time */
    char name[VFS_MAX_NAME_LEN];            /* Entry name */
    VFS_INODE *parent;                      /* Parent directory inode */
    uint32_t permissions;                   /* Access permissions */
    uint32_t owner_uid;                     /* Owner user ID */
    uint32_t owner_gid;                     /* Owner group ID */
} TTYFS_IDENT_ITEM;

/**
 * @brief Input buffer management structure
 */
typedef struct {
    char data[TTY_BUFFER_SIZE];             /* Circular buffer data */
    volatile int64_t head;                  /* Write position (producer) */
    volatile int64_t tail;                  /* Read position (consumer) */
    volatile int64_t count;                 /* Number of bytes in buffer */
    volatile uint32_t flags;                /* Buffer state flags */
    uint32_t high_water;                    /* High water mark for flow control */
    uint32_t low_water;                     /* Low water mark for flow control */
} TTY_INPUT_BUFFER;

/**
 * @brief Output buffer management structure
 */
typedef struct {
    char data[TTY_BUFFER_SIZE];             /* Output buffer data */
    volatile int64_t head;                  /* Write position */
    volatile int64_t tail;                  /* Read position */
    volatile int64_t count;                 /* Number of bytes in buffer */
    volatile uint32_t flags;                /* Buffer state flags */
    bool flow_stopped;                      /* Flow control stopped output */
} TTY_OUTPUT_BUFFER;

/**
 * @brief Process group and session information
 */
typedef struct {
    int32_t session_id;                     /* Session ID */
    int32_t process_group_id;               /* Process group ID (foreground) */
    int32_t controlling_process;            /* Controlling process ID */
    bool is_controlling_tty;                /* Is this the controlling TTY? */
} TTY_SESSION_INFO;

/**
 * @brief TTY device statistics
 */
typedef struct {
    uint64_t bytes_read;                    /* Total bytes read */
    uint64_t bytes_written;                 /* Total bytes written */
    uint64_t read_calls;                    /* Number of read() calls */
    uint64_t write_calls;                   /* Number of write() calls */
    uint64_t ioctl_calls;                   /* Number of ioctl() calls */
    uint64_t interrupts_received;           /* Interrupt characters received */
    uint64_t buffer_overflows;              /* Buffer overflow count */
    uint64_t parity_errors;                 /* Parity error count */
    uint64_t framing_errors;                /* Framing error count */
    STD_TIME last_activity;                 /* Last activity timestamp */
} TTY_STATISTICS;

/**
 * @brief Enhanced TTY identifier structure
 * This is the main structure that represents a TTY device instance
 */
typedef struct {
    /* Legacy buffer fields (for compatibility) */
    char ibuff[TTY_BUFFER_SIZE];            /* Input buffer (legacy) */
    int64_t ibegin;                         /* Input buffer start (legacy) */
    int64_t icursor;                        /* Input buffer cursor (legacy) */
    int64_t isize;                          /* Input buffer size (legacy) */

    /* Enhanced buffer management */
    TTY_INPUT_BUFFER input_buffer;          /* Enhanced input buffer */
    TTY_OUTPUT_BUFFER output_buffer;        /* Output buffer */

    /* Terminal attributes */
    TERMIOS termios;                        /* Terminal I/O settings */
    WINDOW_SIZE winsize;                    /* Terminal window size */

    /* Device state */
    TTY_STATE state;                        /* Current device state */
    TTY_TYPE type;                          /* Device type */
    uint32_t device_flags;                  /* Device-specific flags */
    uint32_t open_count;                    /* Number of open handles */

    /* Process and session management */
    TTY_SESSION_INFO session;               /* Session information */

    /* Flow control and signaling */
    volatile bool line_ready;               /* Complete line available for read */
    volatile bool eof_received;             /* EOF character received */
    volatile bool hangup_pending;           /* Hangup signal pending */
    volatile bool stop_output;              /* Output stopped by flow control */
    volatile bool stop_input;               /* Input stopped by flow control */

    /* Canonical mode line editing */
    char line_buffer[TTY_CANONICAL_MAX_LINE + 1];  /* Current line being edited */
    int64_t line_pos;                       /* Current position in line */
    int64_t line_len;                       /* Current line length */
    bool line_overflow;                     /* Line buffer overflow flag */

    /* Raw mode configuration */
    uint8_t raw_min_chars;                  /* Minimum chars for raw read (VMIN) */
    uint8_t raw_timeout;                    /* Timeout for raw read (VTIME) */

    /* Echo and display control */
    bool echo_enabled;                      /* Echo is currently enabled */
    int64_t column_position;                /* Current cursor column */
    bool at_bol;                           /* At beginning of line */

    /* Statistics and debugging */
    TTY_STATISTICS stats;                   /* Device statistics */

    /* Synchronization */
    void *read_wait_queue;                  /* Processes waiting to read */
    void *write_wait_queue;                 /* Processes waiting to write */
    uint32_t lock_count;                    /* Lock nesting count */

    /* Hardware-specific data */
    void *hw_data;                          /* Hardware-specific data pointer */
    uint32_t hw_data_size;                  /* Size of hardware data */

    /* Error handling */
    int32_t last_error;                     /* Last error code */
    char error_message[128];                /* Last error message */

    /* Performance optimization */
    bool bulk_mode;                         /* Bulk transfer mode enabled */
    uint32_t bulk_threshold;                /* Threshold for bulk operations */

    /* Security and access control */
    uint32_t access_mask;                   /* Allowed access operations */
    bool secure_mode;                       /* Secure mode enabled */

} TTYFS_IDENT;

/**
 * @brief TTY filesystem global state
 */
typedef struct {
    uint32_t device_count;                  /* Number of active devices */
    uint32_t max_devices;                   /* Maximum allowed devices */
    TTYFS_IDENT *devices[TTY_MAX_DEVICES];  /* Array of device pointers */
    uint32_t next_device_id;                /* Next device ID to assign */
    bool initialized;                       /* Filesystem initialized flag */
    void *global_lock;                      /* Global synchronization lock */
} TTYFS_GLOBAL_STATE;

/**
 * @brief TTY operation callbacks
 * These can be used to customize behavior for different TTY types
 */
typedef struct {
    int (*init)(TTYFS_IDENT *tty);          /* Initialize device */
    int (*cleanup)(TTYFS_IDENT *tty);       /* Cleanup device */
    int (*configure)(TTYFS_IDENT *tty);     /* Configure hardware */
    int (*start_tx)(TTYFS_IDENT *tty);      /* Start transmission */
    int (*stop_tx)(TTYFS_IDENT *tty);       /* Stop transmission */
    int (*start_rx)(TTYFS_IDENT *tty);      /* Start reception */
    int (*stop_rx)(TTYFS_IDENT *tty);       /* Stop reception */
    int (*set_break)(TTYFS_IDENT *tty, bool enable);  /* Control break signal */
    int (*get_status)(TTYFS_IDENT *tty, uint32_t *status);  /* Get device status */
} TTY_OPERATIONS;

/**
 * @brief Convenience macros for buffer operations
 */
#define TTY_BUFFER_FULL(buf)    ((buf)->count >= TTY_BUFFER_SIZE - 1)
#define TTY_BUFFER_EMPTY(buf)   ((buf)->count == 0)
#define TTY_BUFFER_SPACE(buf)   (TTY_BUFFER_SIZE - (buf)->count - 1)
#define TTY_BUFFER_AVAILABLE(buf) ((buf)->count)

/**
 * @brief Convenience macros for state checking
 */
#define TTY_IS_OPEN(tty)        ((tty)->state == TTY_STATE_OPEN)
#define TTY_IS_CANONICAL(tty)   ((tty)->termios.c_lflag & ICANON)
#define TTY_IS_RAW(tty)         (!TTY_IS_CANONICAL(tty))
#define TTY_ECHO_ON(tty)        ((tty)->termios.c_lflag & ECHO)
#define TTY_SIGNALS_ON(tty)     ((tty)->termios.c_lflag & ISIG)

/**
 * @brief Function pointer types for TTY operations
 */
typedef int (*tty_init_func_t)(TTYFS_IDENT *tty);
typedef int (*tty_read_func_t)(TTYFS_IDENT *tty, void *buffer, size_t size);
typedef int (*tty_write_func_t)(TTYFS_IDENT *tty, const void *buffer, size_t size);
typedef int (*tty_ioctl_func_t)(TTYFS_IDENT *tty, int cmd, void *arg);

/**
 * @brief TTY device registration structure
 */
typedef struct {
    const char *name;                       /* Device name template */
    TTY_TYPE type;                          /* Device type */
    TTY_OPERATIONS *ops;                    /* Operation callbacks */
    uint32_t flags;                         /* Registration flags */
    void *driver_data;                      /* Driver-specific data */
} TTY_DEVICE_REGISTRATION;

/**
 * @brief TTY registration flags
 */
#define TTY_REG_CONSOLE         0x001       /* Register as console device */
#define TTY_REG_SERIAL          0x002       /* Register as serial device */
#define TTY_REG_PTY_MASTER      0x004       /* Register as PTY master */
#define TTY_REG_PTY_SLAVE       0x008       /* Register as PTY slave */
#define TTY_REG_VIRTUAL         0x010       /* Register as virtual terminal */
#define TTY_REG_NETWORK         0x020       /* Register as network terminal */
#define TTY_REG_EXCLUSIVE       0x040       /* Exclusive access device */
#define TTY_REG_PERSISTENT      0x080       /* Persistent device */

/**
 * @brief TTY capability flags for advanced features
 */
#define TTY_CAP_HARDWARE_FLOW   0x001       /* Hardware flow control */
#define TTY_CAP_SOFTWARE_FLOW   0x002       /* Software flow control */
#define TTY_CAP_BREAK_DETECTION 0x004       /* Break signal detection */
#define TTY_CAP_PARITY_CHECK    0x008       /* Parity checking */
#define TTY_CAP_MODEM_CONTROL   0x010       /* Modem control signals */
#define TTY_CAP_CARRIER_DETECT  0x020       /* Carrier detection */
#define TTY_CAP_RING_INDICATOR  0x040       /* Ring indicator */
#define TTY_CAP_DMA_SUPPORT     0x080       /* DMA transfer support */
#define TTY_CAP_FIFO_BUFFER     0x100       /* Hardware FIFO buffers */
#define TTY_CAP_HIGH_SPEED      0x200       /* High-speed operation */

/**
 * @brief TTY interrupt types
 */
typedef enum {
    TTY_INT_RX_READY = 0,                   /* Receive data ready */
    TTY_INT_TX_EMPTY,                       /* Transmit buffer empty */
    TTY_INT_LINE_STATUS,                    /* Line status change */
    TTY_INT_MODEM_STATUS,                   /* Modem status change */
    TTY_INT_BREAK_DETECT,                   /* Break signal detected */
    TTY_INT_PARITY_ERROR,                   /* Parity error */
    TTY_INT_FRAME_ERROR,                    /* Framing error */
    TTY_INT_OVERRUN_ERROR,                  /* Buffer overrun */
    TTY_INT_TIMEOUT,                        /* Receive timeout */
    TTY_INT_WAKEUP,                         /* Wakeup interrupt */
    TTY_INT_COUNT                           /* Number of interrupt types */
} TTY_INTERRUPT_TYPE;

/**
 * @brief TTY interrupt handler function type
 */
typedef void (*tty_interrupt_handler_t)(TTYFS_IDENT *tty, TTY_INTERRUPT_TYPE type, void *data);

/**
 * @brief Extended TTY operations structure
 */
typedef struct {
    TTY_OPERATIONS base;                    /* Base operations */

    /* Advanced I/O operations */
    int (*bulk_read)(TTYFS_IDENT *tty, void *buffer, size_t size, size_t *transferred);
    int (*bulk_write)(TTYFS_IDENT *tty, const void *buffer, size_t size, size_t *transferred);
    int (*async_read)(TTYFS_IDENT *tty, void *buffer, size_t size, void *callback);
    int (*async_write)(TTYFS_IDENT *tty, const void *buffer, size_t size, void *callback);

    /* Power management */
    int (*suspend)(TTYFS_IDENT *tty);
    int (*resume)(TTYFS_IDENT *tty);
    int (*set_power_state)(TTYFS_IDENT *tty, uint32_t state);

    /* Flow control */
    int (*set_flow_control)(TTYFS_IDENT *tty, bool enable);
    int (*send_xon)(TTYFS_IDENT *tty);
    int (*send_xoff)(TTYFS_IDENT *tty);

    /* Modem control */
    int (*set_dtr)(TTYFS_IDENT *tty, bool state);
    int (*set_rts)(TTYFS_IDENT *tty, bool state);
    int (*get_dcd)(TTYFS_IDENT *tty, bool *state);
    int (*get_dsr)(TTYFS_IDENT *tty, bool *state);
    int (*get_cts)(TTYFS_IDENT *tty, bool *state);
    int (*get_ri)(TTYFS_IDENT *tty, bool *state);

    /* Interrupt handling */
    int (*register_interrupt)(TTYFS_IDENT *tty, TTY_INTERRUPT_TYPE type, tty_interrupt_handler_t handler);
    int (*unregister_interrupt)(TTYFS_IDENT *tty, TTY_INTERRUPT_TYPE type);
    int (*enable_interrupt)(TTYFS_IDENT *tty, TTY_INTERRUPT_TYPE type);
    int (*disable_interrupt)(TTYFS_IDENT *tty, TTY_INTERRUPT_TYPE type);

    /* Buffer management */
    int (*flush_input)(TTYFS_IDENT *tty);
    int (*flush_output)(TTYFS_IDENT *tty);
    int (*get_input_count)(TTYFS_IDENT *tty, size_t *count);
    int (*get_output_count)(TTYFS_IDENT *tty, size_t *count);

    /* Diagnostics and testing */
    int (*self_test)(TTYFS_IDENT *tty, uint32_t *result);
    int (*loopback_test)(TTYFS_IDENT *tty, const void *data, size_t size);
    int (*get_diagnostics)(TTYFS_IDENT *tty, void *diag_data, size_t size);

} TTY_EXTENDED_OPERATIONS;

/**
 * @brief TTY configuration structure for initialization
 */
typedef struct {
    const char *device_name;                /* Device name */
    TTY_TYPE device_type;                   /* Device type */
    uint32_t buffer_size;                   /* Buffer size override */
    uint32_t capabilities;                  /* Device capabilities */
    uint32_t default_baud;                  /* Default baud rate */
    uint8_t default_data_bits;              /* Default data bits */
    uint8_t default_stop_bits;              /* Default stop bits */
    uint8_t default_parity;                 /* Default parity */
    bool default_flow_control;              /* Default flow control */
    TTY_EXTENDED_OPERATIONS *ext_ops;       /* Extended operations */
    void *hw_config;                        /* Hardware-specific config */
} TTY_CONFIG;

/**
 * @brief TTY event structure for notifications
 */
typedef struct {
    TTY_INTERRUPT_TYPE event_type;          /* Type of event */
    TTYFS_IDENT *tty;                       /* TTY device */
    uint64_t timestamp;                     /* Event timestamp */
    uint32_t data_length;                   /* Length of event data */
    void *event_data;                       /* Event-specific data */
} TTY_EVENT;

/**
 * @brief TTY event handler function type
 */
typedef void (*tty_event_handler_t)(const TTY_EVENT *event, void *user_data);

/**
 * @brief Function prototypes for TTY filesystem operations
 * These match the VFS interface requirements
 */

/* Core filesystem operations */
void init_ttyfs(void);
VFS_INODE *ttyfs_mount(VFS_INODE *at);
VFS_TNODE *ttyfs_open(VFS_INODE *this, const char *path);
int64_t ttyfs_mknode(VFS_TNODE *this);
int64_t ttyfs_rmnode(VFS_TNODE *this);

/* I/O operations */
int64_t ttyfs_read(VFS_INODE *this, size_t offset, size_t len, void *buff);
int64_t ttyfs_write(VFS_INODE *this, size_t offset, size_t len, const void *buff);
int64_t ttyfs_ioctl(VFS_INODE *this, int64_t request, int64_t arg);

/* Management operations */
int64_t ttyfs_sync(VFS_INODE *this);
int64_t ttyfs_refresh(VFS_INODE *this);
int64_t ttyfs_getdent(VFS_INODE *this, size_t pos, VFS_DIR_ENTRY *dirent);

/* Extended TTY management functions */
int tty_register_device(const TTY_DEVICE_REGISTRATION *reg, TTYFS_IDENT **tty_out);
int tty_unregister_device(TTYFS_IDENT *tty);
int tty_configure_device(TTYFS_IDENT *tty, const TTY_CONFIG *config);
int tty_get_device_by_name(const char *name, TTYFS_IDENT **tty_out);
int tty_get_device_by_id(uint32_t device_id, TTYFS_IDENT **tty_out);

/* Event management functions */
int tty_register_event_handler(TTYFS_IDENT *tty, TTY_INTERRUPT_TYPE event_type,
                              tty_event_handler_t handler, void *user_data);
int tty_unregister_event_handler(TTYFS_IDENT *tty, TTY_INTERRUPT_TYPE event_type);
int tty_post_event(TTYFS_IDENT *tty, const TTY_EVENT *event);

/* Utility functions */
int tty_validate_termios(const TERMIOS *termios);
int tty_copy_termios(TERMIOS *dest, const TERMIOS *src);
int tty_reset_termios(TERMIOS *termios);
const char *tty_get_type_name(TTY_TYPE type);
const char *tty_get_state_name(TTY_STATE state);

/* Buffer management utilities */
int tty_buffer_init(TTY_INPUT_BUFFER *buffer);
int tty_buffer_cleanup(TTY_INPUT_BUFFER *buffer);
int tty_buffer_put(TTY_INPUT_BUFFER *buffer, uint8_t data);
int tty_buffer_get(TTY_INPUT_BUFFER *buffer, uint8_t *data);
int tty_buffer_peek(TTY_INPUT_BUFFER *buffer, uint8_t *data, size_t offset);
size_t tty_buffer_available(const TTY_INPUT_BUFFER *buffer);
size_t tty_buffer_space(const TTY_INPUT_BUFFER *buffer);
int tty_buffer_flush(TTY_INPUT_BUFFER *buffer);

/* Security and access control */
int tty_check_access(TTYFS_IDENT *tty, uint32_t requested_access);
int tty_set_access_mask(TTYFS_IDENT *tty, uint32_t access_mask);
int tty_enable_secure_mode(TTYFS_IDENT *tty, bool enable);

/* Statistics and monitoring */
int tty_get_statistics(TTYFS_IDENT *tty, TTY_STATISTICS *stats);
int tty_reset_statistics(TTYFS_IDENT *tty);
int tty_dump_state(TTYFS_IDENT *tty, char *buffer, size_t buffer_size);

/**
 * @brief Global TTY filesystem state (external declaration)
 */
extern TTYFS_GLOBAL_STATE ttyfs_global_state;

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