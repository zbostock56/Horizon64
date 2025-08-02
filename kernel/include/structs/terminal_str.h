/**
 * @file terminal_str.h
 * @author Zack Bostock
 * @brief Structs pertaining to the terminal
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#include <structs/framebuffer_str.h>
#include <structs/termios_str.h>
#include <graphics/graphics.h>
#include <stdint.h>
#include <stdbool.h>

/* Terminal boolean definitions */
#define TERM_IS_BOLD        (1)
#define TERM_NOT_BOLD       (0)
#define TERM_TRUE           (1)
#define TERM_FALSE          (0)

/* Terminal control constants */
#define TERMINAL_CURSOR_BLINK_RATE  500     /* How long it takes for cursor to blink (ms) */
#define TERMINAL_BELL_DURATION      100     /* How long the bell rings (ms) */
#define TERMINAL_SCROLL_MARGIN      1
#define MAX_CSI_PARAMS              16
#define MAX_CSI_INTERMEDIATES       4
#define MAX_OSC_BUFFER              256

/* Terminal constants */
#define TERMINAL_DEFAULT_WIDTH      80      /* Default terminal width in chars */
#define TERMINAL_DEFAULT_HEIGHT     25      /* Default terminal height in chars */
#define TERMINAL_MAX_WIDTH          200     /* Maximum terminal width */
#define TERMINAL_MAX_HEIGHT         100     /* Maximum terminal height */
#define TERMINAL_MIN_WIDTH          20      /* Minimum terminal width */
#define TERMINAL_MIN_HEIGHT         5       /* Minimum terminal height */
#define TERMINAL_DEFAULT_TAB_WIDTH  8       /* Default tab width */
#define TERMINAL_MAX_PARAMS         16      /* Maximum escape sequence parameters */
#define TERMINAL_BUFFER_SIZE        4096    /* Terminal I/O buffer size */
#define MAX_TITLE_LENGTH            256
#define MAX_ICON_NAME_LENGTH        100

/* Terminal feature flags */
#define TERM_FEATURE_AUTO_WRAP      0x001   /* Automatic line wrapping */
#define TERM_FEATURE_CURSOR_KEYS    0x002   /* Cursor key application mode */
#define TERM_FEATURE_KEYPAD         0x004   /* Keypad application mode */
#define TERM_FEATURE_INSERT_MODE    0x008   /* Insert/replace mode */
#define TERM_FEATURE_ORIGIN_MODE    0x010   /* Origin mode (relative to scroll region) */
#define TERM_FEATURE_NEWLINE_MODE   0x020   /* Newline mode */
#define TERM_FEATURE_REVERSE_WRAP   0x040   /* Reverse wraparound */
#define TERM_FEATURE_SMOOTH_SCROLL  0x080   /* Smooth scrolling */

/* Enhanced terminal capability flags */
#define TERM_CAP_COLOR          0x001       /* Color support */
#define TERM_CAP_RESIZE         0x080       /* Dynamic resizing */
#define TERM_CAP_UTF8           0x100       /* UTF-8 support */
#define TERM_CAP_MOUSE          0x200       /* Mouse support */
#define TERM_CAP_CLIPBOARD      0x400       /* Clipboard support */
#define TERM_CAP_HYPERLINKS     0x800       /* Hyperlink support */

/* ANSI Escape Sequence constants */
#define ESC_START       '\033'
#define OSC_START       '\007'
#define CSI_BRACKET     '['
#define OSC_BRACKET     ']'

/* Default colors */
#ifndef DEFAULT_FG
#define DEFAULT_FG      COLOR_WHITE
#endif
#ifndef DEFAULT_BG
#define DEFAULT_BG      COLOR_BLACK
#endif

typedef uint8_t TERM_BOLD;

/**
 * @brief Terminal operation modes
 */
typedef enum {
    TERM_MODE_TERM = 0x0,    /* CLI terminal mode */
    TERM_MODE_GUI,           /* GUI mode */
    TERM_MODE_INFO,          /* Info/debug mode */
    TERM_MODE_UNSET,         /* Uninitialized mode */
} TERM_MODE;

/**
 * @brief Terminal cursor visibility states
 */
typedef enum {
    TERM_CURSOR_INVISIBLE = 0x0,
    TERM_CURSOR_VISIBLE,
    TERM_CURSOR_HIDE,
    TERM_CURSOR_BLOCK
} TERM_CURSOR_STATUS;

/**
 * @brief Terminal parsing states for ANSI sequences
 */
typedef enum {
    TERM_STATE_UNKNOWN = 0x0,
    TERM_STATE_IDLE,
    TERM_STATE_CMD,
    TERM_STATE_PARAM,
    TERM_STATE_HYPERLINK_HEADER,
    TERM_STATE_HYPERLINK_URL,
    TERM_STATE_HYPERLINK_TEXT,
    TERM_STATE_HYPERLINK_TAIL,
    TERM_STATE_OSC,
    TERM_STATE_CSI
} TERM_STATE;

/**
 * @brief Terminal character sets
 */
typedef enum {
    TERM_CHARSET_ASCII = 0,     /* Standard ASCII */
    TERM_CHARSET_UTF8,          /* UTF-8 Unicode */
    TERM_CHARSET_LATIN1,        /* ISO 8859-1 */
    TERM_CHARSET_CP437,         /* Code Page 437 (IBM PC) */
    TERM_CHARSET_COUNT          /* Number of character sets */
} TERM_CHARSET;

/**
 * @brief Terminal operation types for statistics
 */
typedef enum {
    TERM_OP_CHAR_WRITE = 0,     /* Character written */
    TERM_OP_LINE_FEED,          /* Line feed operation */
    TERM_OP_SCROLL,             /* Scroll operation */
    TERM_OP_ESCAPE_SEQ,         /* Escape sequence processed */
    TERM_OP_BELL,               /* Bell/beep operation */
    TERM_OP_CLEAR,              /* Clear operation */
    TERM_OP_ERROR,              /* Error occurred */
    TERM_OP_COUNT               /* Number of operation types */
} TERM_OPERATION;

/**
 * @brief Terminal statistics structure
 */
typedef struct {
    uint64_t chars_written;         /* Total characters written */
    uint64_t lines_written;         /* Total lines written */
    uint64_t scroll_operations;     /* Number of scroll operations */
    uint64_t escape_sequences;      /* Escape sequences processed */
    uint64_t clear_operations;      /* Clear operations performed */
    uint64_t bell_operations;       /* Bell operations */
    uint64_t errors;                /* Number of errors */
    uint64_t last_activity_time;    /* Timestamp of last activity */
} TERMINAL_STATS;

/**
 * @brief Global terminal statistics
 */
typedef struct {
    uint64_t total_chars_written;
    uint64_t total_lines_written;
    uint64_t total_scroll_operations;
    uint64_t total_escape_sequences;
    uint64_t total_errors;
    uint64_t total_bell_operations;
    uint64_t total_clear_operations;
    uint64_t total_cursor_moves;
    uint64_t total_color_changes;
    uint64_t total_operations;
} GLOBAL_TERMINAL_STATS;

/**
 * @brief Terminal buffer for input/output operations
 */
typedef struct {
    char *data;
    size_t size;
    size_t capacity;
    size_t read_pos;
    size_t write_pos;
} TERMINAL_BUFFER;

/**
 * @brief Terminal tab stop structure
 */
typedef struct {
    uint8_t stops[256];    /* Tab stops array */
    int default_width;  /* Default tab width */
} TAB_STOPS;

/**
 * @brief Terminal color palette
 */
typedef struct {
    uint32_t colors[256];      /* 256-color palette */
    uint8_t custom[256];       /* Track custom colors */
} COLOR_PALETTE;

/**
 * @brief Enhanced terminal structure with full ANSI support
 */
typedef struct {
    /* Display properties */
    FRAMEBUFFER framebuffer;
    uint32_t background_color;
    uint32_t foreground_color;
    uint32_t width;             /* In units of characters, not pixels */
    uint32_t height;            /* In units of characters, not pixels */

    /* Cursor management */
    IVEC2 cursor_pos;           /* Current cursor position (chars) */
    IVEC2 saved_cursor_pos;     /* Saved cursor position */
    uint8_t cursor_visible;        /* Cursor visibility flag */
    TERM_CURSOR_STATUS cursor_type; /* Cursor type */

    /* Terminal state */
    TERM_MODE mode;             /* Current terminal mode */
    TERM_STATE state;           /* Current parsing state */
    TERM_CHARSET charset;       /* Character set */

    /* Text attributes */
    TERM_BOLD is_bold;          /* Bold text flag */
    uint8_t is_underline;          /* Underline flag */
    uint8_t is_reverse;            /* Reverse video flag */
    uint8_t is_italic;             /* Italic flag */
    uint8_t is_strikethrough;      /* Strikethrough flag */
    uint8_t is_blink;              /* Blink flag */

    /* Terminal behavior flags */
    uint8_t auto_wrap;             /* Auto line wrap */
    uint8_t insert_mode;           /* Insert vs replace mode */
    uint8_t origin_mode;           /* Origin mode (relative to scroll region) */
    uint8_t newline_mode;          /* LF vs CRLF mode */
    uint8_t echo_mode;             /* Echo input characters */
    uint8_t canonical_mode;        /* Line-buffered input */

    /* Scroll region */
    int scroll_top;             /* Top of scroll region */
    int scroll_bottom;          /* Bottom of scroll region */

    /* Tab handling */
    TAB_STOPS tab_stops;        /* Tab stop configuration */
    int tab_width;              /* Default tab width */

    /* ANSI sequence parsing */
    int cparams[TERMINAL_MAX_PARAMS];   /* CSI parameters */
    int cparamcount;                    /* Number of parameters */
    char csi_intermediates[MAX_CSI_INTERMEDIATES]; /* CSI intermediate chars */
    int csi_intermediate_count;

    /* OSC sequence handling */
    char osc_buffer[MAX_OSC_BUFFER];    /* OSC command buffer */
    size_t osc_buffer_len;              /* OSC buffer length */
    uint8_t osc_expecting_backslash;       /* OSC terminator state */

    /* UTF-8 handling */
    uint8_t last_char;          /* Last processed character */
    uint8_t last_qu_char;       /* Last quote character flag */
    uint8_t skip_left;          /* UTF-8 bytes remaining */
    uint32_t utf8_codepoint;    /* Current UTF-8 codepoint */
    int utf8_state;             /* UTF-8 decoder state */

    /* Color management */
    COLOR_PALETTE palette;      /* Color palette */
    uint32_t saved_fg_color;    /* Saved foreground color */
    uint32_t saved_bg_color;    /* Saved background color */

    /* Window management */
    char window_title[MAX_TITLE_LENGTH];    /* Window title */
    char icon_name[MAX_ICON_NAME_LENGTH];   /* Icon name */

    /* Input/Output buffers */
    TERMINAL_BUFFER input_buffer;   /* Input buffer */
    TERMINAL_BUFFER output_buffer;  /* Output buffer */

    /* Termios compatibility */
    TERMIOS termios;
    uint8_t termios_enabled;

    /* Statistics */
    TERMINAL_STATS stats;       /* Terminal statistics */

    /* Timing and performance */
    uint64_t last_update_time;  /* Last screen update time */
    uint32_t refresh_rate;      /* Target refresh rate */
    uint8_t dirty;                 /* Screen needs refresh */

    /* Bell/audio */
    uint8_t bell_enabled;          /* Bell enabled flag */
    uint32_t bell_frequency;    /* Bell frequency */
    uint32_t bell_duration;     /* Bell duration */

    /* Advanced features */
    uint8_t hyperlinks_enabled;    /* Hyperlink support */
    uint8_t mouse_tracking;        /* Mouse tracking mode */
    uint8_t alternate_screen;      /* Alternate screen buffer */
    uint8_t bracketed_paste;       /* Bracketed paste mode */
} TERMINAL;

/**
 * @brief Terminal capability flags
 */
typedef struct {
    uint8_t ansi_colors;           /* ANSI color support */
    uint8_t extended_colors;       /* 256-color support */
    uint8_t rgb_colors;            /* RGB color support */
    uint8_t unicode;               /* Unicode support */
    uint8_t mouse;                 /* Mouse support */
    uint8_t hyperlinks;            /* Hyperlink support */
    uint8_t sixel;                 /* Sixel graphics support */
    uint8_t italic;                /* Italic text support */
    uint8_t underline;             /* Underline support */
    uint8_t strikethrough;         /* Strikethrough support */
    uint8_t blink;                 /* Blink support */
} TERMINAL_CAPS;

/**
 * @brief Terminal event structure
 */
typedef enum {
    TERM_EVENT_KEY_PRESS,
    TERM_EVENT_KEY_RELEASE,
    TERM_EVENT_MOUSE_MOVE,
    TERM_EVENT_MOUSE_CLICK,
    TERM_EVENT_MOUSE_RELEASE,
    TERM_EVENT_RESIZE,
    TERM_EVENT_FOCUS_IN,
    TERM_EVENT_FOCUS_OUT,
    TERM_EVENT_BELL,
    TERM_EVENT_TITLE_CHANGE
} TERM_EVENT_TYPE;

typedef struct {
    TERM_EVENT_TYPE type;
    uint64_t timestamp;
    union {
        struct {
            uint32_t keycode;
            uint32_t modifiers;
            char utf8[8];
        } key;
        struct {
            int x, y;
            uint32_t buttons;
        } mouse;
        struct {
            uint32_t width, height;
        } resize;
        struct {
            char title[MAX_TITLE_LENGTH];
        } title;
    } data;
} TERMINAL_EVENT;

/**
 * @brief Terminal configuration structure
 */
typedef struct {
    /* Display settings */
    uint32_t default_fg_color;
    uint32_t default_bg_color;
    uint32_t font_size;
    uint8_t double_buffering;

    /* Behavior settings */
    uint8_t auto_wrap_default;
    uint8_t cursor_blink;
    uint32_t cursor_blink_rate;
    uint32_t scroll_buffer_size;

    /* Feature flags */
    TERMINAL_CAPS capabilities;

    /* Performance settings */
    uint32_t max_refresh_rate;
    uint8_t vsync_enabled;

    /* Input settings */
    uint8_t mouse_enabled;
    uint8_t keyboard_enabled;
    uint32_t input_timeout;
} TERMINAL_CONFIG;

/* Function pointer types for terminal operations */
typedef void (*terminal_bell_callback_t)(TERMINAL *term);
typedef void (*terminal_resize_callback_t)(TERMINAL *term, uint32_t width, uint32_t height);
typedef void (*terminal_title_callback_t)(TERMINAL *term, const char *title);

/**
 * @brief Terminal callback structure
 */
typedef struct {
    terminal_bell_callback_t bell_callback;
    terminal_resize_callback_t resize_callback;
    terminal_title_callback_t title_callback;
} TERMINAL_CALLBACKS;

/* Compatibility macros for existing code */
#define TERM_CURSOR_HIDE TERM_CURSOR_INVISIBLE

/* Status codes for terminal operations */
#ifndef STATUS_DEFINED
#define STATUS_DEFINED
typedef enum {
    TERM_OK = 0,
    TERM_ERR = -1,
    TERM_TIMEOUT = -2,
    TERM_INVALID_PARAM = -3,
    TERM_OUT_OF_MEMORY = -4,
    TERM_NOT_SUPPORTED = -5
} TERM_STATUS;
#endif

/* Color constants (if not defined elsewhere) */
#ifndef COLOR_BLACK
#define COLOR_BLACK     0x00000000
#define COLOR_RED       0x00FF0000
#define COLOR_GREEN     0x0000FF00
#define COLOR_YELLOW    0x00FFFF00
#define COLOR_BLUE      0x000000FF
#define COLOR_MAGENTA   0x00FF00FF
#define COLOR_CYAN      0x0000FFFF
#define COLOR_WHITE     0x00FFFFFF
#define COLOR_BRIGHT_BLACK   0x00808080
#define COLOR_BRIGHT_RED     0x00FF8080
#define COLOR_BRIGHT_GREEN   0x0080FF80
#define COLOR_BRIGHT_YELLOW  0x00FFFF80
#define COLOR_BRIGHT_BLUE    0x008080FF
#define COLOR_BRIGHT_MAGENTA 0x00FF80FF
#define COLOR_BRIGHT_CYAN    0x0080FFFF
#define COLOR_BRIGHT_WHITE   0x00FFFFFF
#endif