/**
 * @file termios_str.h
 * @author Zack Bostock
 * @brief Structures for a general terminal interface 
 * @ref https://pubs.opengroup.org/onlinepubs/7908799/xsh/termios.h.html
 * @ref https://www.man7.org/linux/man-pages/man3/termios.3.html
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#pragma once
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Standard ioctl commands for terminal control
 */
#define TCGETS          0x5401  /* Get terminal attributes */
#define TCSETS          0x5402  /* Set terminal attributes */
#define TCSETSW         0x5403  /* Set terminal attributes after draining output */
#define TCSETSF         0x5404  /* Set terminal attributes after draining output and flushing input */
#define TCGETA          0x5405  /* Get terminal attributes (BSD compatibility) */
#define TCSETA          0x5406  /* Set terminal attributes (BSD compatibility) */
#define TCSETAW         0x5407  /* Set terminal attributes, wait (BSD compatibility) */
#define TCSETAF         0x5408  /* Set terminal attributes, flush (BSD compatibility) */
#define TCSBRK          0x5409  /* Send break */
#define TCXONC          0x540A  /* Flow control */
#define TCFLSH          0x540B  /* Flush queues */

/**
 * @brief Terminal window and process group control
 */
#define TIOCGPGRP       0x540F  /* Get process group */
#define TIOCSPGRP       0x5410  /* Set process group */
#define TIOCGSID        0x5429  /* Get session ID */
#define TIOCGWINSZ      0x5413  /* Get window size */
#define TIOCSWINSZ      0x5414  /* Set window size */

/**
 * @brief Advanced terminal control commands
 */
#define TIOCSTI         0x5412  /* Simulate terminal input */
#define TIOCMGET        0x5415  /* Get modem status */
#define TIOCMSET        0x5418  /* Set modem status */
#define TIOCMBIC        0x5417  /* Clear modem status bits */
#define TIOCMBIS        0x5416  /* Set modem status bits */
#define TIOCOUTQ        0x5411  /* Get output queue size */

/**
 * @brief Input mode flags (c_iflag)
 */
#define IGNBRK          0x00000001  /* Ignore break condition */
#define BRKINT          0x00000002  /* Signal interrupt on break */
#define IGNPAR          0x00000004  /* Ignore characters with parity errors */
#define PARMRK          0x00000008  /* Mark parity and framing errors */
#define INPCK           0x00000010  /* Enable input parity check */
#define ISTRIP          0x00000020  /* Strip 8th bit off characters */
#define INLCR           0x00000040  /* Map NL to CR on input */
#define IGNCR           0x00000080  /* Ignore CR */
#define ICRNL           0x00000100  /* Map CR to NL on input */
#define IUCLC           0x00000200  /* Map uppercase to lowercase on input */
#define IXON            0x00000400  /* Enable XON/XOFF flow control on output */
#define IXANY           0x00000800  /* Enable any character to restart output */
#define IXOFF           0x00001000  /* Enable XON/XOFF flow control on input */
#define IMAXBEL         0x00002000  /* Ring bell on input queue full */
#define IUTF8           0x00004000  /* Input is UTF8 */

/**
 * @brief Output mode flags (c_oflag)
 */
#define OPOST           0x00000001  /* Post-process output */
#define OLCUC           0x00000002  /* Map lowercase to uppercase on output */
#define ONLCR           0x00000004  /* Map NL to CR-NL on output */
#define OCRNL           0x00000008  /* Map CR to NL on output */
#define ONOCR           0x00000010  /* No CR output at column 0 */
#define ONLRET          0x00000020  /* NL performs CR function */
#define OFILL           0x00000040  /* Use fill characters for delay */
#define OFDEL           0x00000080  /* Fill is DEL, else NUL */
#define NLDLY           0x00000100  /* Select newline delays */
#define NL0             0x00000000  /* Newline type 0 */
#define NL1             0x00000100  /* Newline type 1 */
#define CRDLY           0x00000600  /* Select carriage-return delays */
#define CR0             0x00000000  /* Carriage-return delay type 0 */
#define CR1             0x00000200  /* Carriage-return delay type 1 */
#define CR2             0x00000400  /* Carriage-return delay type 2 */
#define CR3             0x00000600  /* Carriage-return delay type 3 */
#define TABDLY          0x00001800  /* Select horizontal-tab delays */
#define TAB0            0x00000000  /* Horizontal-tab delay type 0 */
#define TAB1            0x00000800  /* Horizontal-tab delay type 1 */
#define TAB2            0x00001000  /* Horizontal-tab delay type 2 */
#define TAB3            0x00001800  /* Expand tabs to spaces */
#define BSDLY           0x00002000  /* Select backspace delays */
#define BS0             0x00000000  /* Backspace-delay type 0 */
#define BS1             0x00002000  /* Backspace-delay type 1 */
#define VTDLY           0x00004000  /* Select vertical-tab delays */
#define VT0             0x00000000  /* Vertical-tab delay type 0 */
#define VT1             0x00004000  /* Vertical-tab delay type 1 */
#define FFDLY           0x00008000  /* Select form-feed delays */
#define FF0             0x00000000  /* Form-feed delay type 0 */
#define FF1             0x00008000  /* Form-feed delay type 1 */

/**
 * @brief Control mode flags (c_cflag)
 */
#define CSIZE           0x00000030  /* Character size mask */
#define CS5             0x00000000  /* 5 bits */
#define CS6             0x00000010  /* 6 bits */
#define CS7             0x00000020  /* 7 bits */
#define CS8             0x00000030  /* 8 bits */
#define CSTOPB          0x00000040  /* Send two stop bits */
#define CREAD           0x00000080  /* Enable receiver */
#define PARENB          0x00000100  /* Parity enable */
#define PARODD          0x00000200  /* Odd parity */
#define HUPCL           0x00000400  /* Hang up on last close */
#define CLOCAL          0x00000800  /* Ignore modem status lines */
#define CBAUD           0x0000100F  /* Baud speed mask */
#define CBAUDEX         0x00001000  /* Extra baud speed mask */
#define CIBAUD          0x002F0000  /* Input baud rate mask */
#define CMSPAR          0x40000000  /* Mark or space parity */
#define CRTSCTS         0x80000000  /* Flow control */

/**
 * @brief Local mode flags (c_lflag)
 */
#define ISIG            0x00000001  /* Enable signals */
#define ICANON          0x00000002  /* Canonical input (erase and kill processing) */
#define XCASE           0x00000004  /* Canonical upper/lower case */
#define ECHO            0x00000008  /* Enable echo */
#define ECHOE           0x00000010  /* Echo erase character as error-correcting backspace */
#define ECHOK           0x00000020  /* Echo kill */
#define ECHONL          0x00000040  /* Echo NL */
#define NOFLSH          0x00000080  /* Disable flush after interrupt or quit */
#define TOSTOP          0x00000100  /* Send SIGTTOU for background output */
#define ECHOCTL         0x00000200  /* Echo control characters as ^X */
#define ECHOPRT         0x00000400  /* Echo erased characters backwards */
#define ECHOKE          0x00000800  /* Echo kill by erasing each character on line */
#define FLUSHO          0x00001000  /* Output being flushed */
#define PENDIN          0x00004000  /* Retype pending input */
#define IEXTEN          0x00008000  /* Enable extended input character processing */
#define EXTPROC         0x00010000  /* External processing */

/**
 * @brief Control character indices for c_cc array
 */
#define NCCS            32          /* Size of control character array */
#define VINTR           0           /* Interrupt character */
#define VQUIT           1           /* Quit character */
#define VERASE          2           /* Erase character */
#define VKILL           3           /* Kill character */
#define VEOF            4           /* End-of-file character */
#define VTIME           5           /* Time-out value (tenths of a second) */
#define VMIN            6           /* Minimum number of bytes read at once */
#define VSWTC           7           /* Switch character */
#define VSTART          8           /* Start character */
#define VSTOP           9           /* Stop character */
#define VSUSP           10          /* Suspend character */
#define VEOL            11          /* End-of-line character */
#define VREPRINT        12          /* Reprint-line character */
#define VDISCARD        13          /* Discard character */
#define VWERASE         14          /* Word-erase character */
#define VLNEXT          15          /* Literal-next character */
#define VEOL2           16          /* Second end-of-line character */

/**
 * @brief Special values for VMIN and VTIME
 */
#define _POSIX_VDISABLE 0           /* Disable special character functions */

/**
 * @brief Standard baud rates
 */
#define B0              0x00000000
#define B50             0x00000001
#define B75             0x00000002
#define B110            0x00000003
#define B134            0x00000004
#define B150            0x00000005
#define B200            0x00000006
#define B300            0x00000007
#define B600            0x00000008
#define B1200           0x00000009
#define B1800           0x0000000A
#define B2400           0x0000000B
#define B4800           0x0000000C
#define B9600           0x0000000D
#define B19200          0x0000000E
#define B38400          0x0000000F
#define B57600          0x00001001
#define B115200         0x00001002
#define B230400         0x00001003
#define B460800         0x00001004
#define B500000         0x00001005
#define B576000         0x00001006
#define B921600         0x00001007
#define B1000000        0x00001008
#define B1152000        0x00001009
#define B1500000        0x0000100A
#define B2000000        0x0000100B
#define B2500000        0x0000100C
#define B3000000        0x0000100D
#define B3500000        0x0000100E
#define B4000000        0x0000100F

/**
 * @brief Actions for tcflush()
 */
#define TCIFLUSH        0           /* Flush pending input */
#define TCOFLUSH        1           /* Flush untransmitted output */
#define TCIOFLUSH       2           /* Flush both pending input and untransmitted output */

/**
 * @brief Actions for tcflow()
 */
#define TCOOFF          0           /* Suspend output */
#define TCOON           1           /* Restart suspended output */
#define TCIOFF          2           /* Send STOP character */
#define TCION           3           /* Send START character */

/**
 * @brief Optional actions for tcsetattr()
 */
#define TCSANOW         0           /* Change attributes immediately */
#define TCSADRAIN       1           /* Change attributes when output has drained */
#define TCSAFLUSH       2           /* Change attributes when output has drained; also flush pending input */

/**
 * @brief Type definitions for terminal interface
 */
typedef uint8_t     CC;             /* Control character type */
typedef uint32_t    SPEED;          /* Baud rate type */
typedef uint32_t    TCFLAG;         /* Terminal control flag type */

/**
 * @brief Window size structure for TIOCGWINSZ/TIOCSWINSZ
 */
typedef struct {
    uint16_t row;           /* Rows, in characters */
    uint16_t col;           /* Columns, in characters */
    uint16_t xpixel;        /* Horizontal size, in pixels */
    uint16_t ypixel;        /* Vertical size, in pixels */
} WINDOW_SIZE;

/**
 * @brief Main termios structure
 * This structure defines the terminal interface according to POSIX standards
 */
typedef struct {
    TCFLAG c_iflag;                 /* Input mode flags */
    TCFLAG c_oflag;                 /* Output mode flags */
    TCFLAG c_cflag;                 /* Control mode flags */
    TCFLAG c_lflag;                 /* Local mode flags */
    CC c_cc[NCCS];                  /* Control characters */
    SPEED c_ispeed;                 /* Input baud rate */
    SPEED c_ospeed;                 /* Output baud rate */
} TERMIOS;

/**
 * @brief Modem status bits for TIOCMGET/TIOCMSET
 */
#define TIOCM_LE        0x001       /* Line enable */
#define TIOCM_DTR       0x002       /* Data terminal ready */
#define TIOCM_RTS       0x004       /* Request to send */
#define TIOCM_ST        0x008       /* Secondary transmit */
#define TIOCM_SR        0x010       /* Secondary receive */
#define TIOCM_CTS       0x020       /* Clear to send */
#define TIOCM_CAR       0x040       /* Carrier detect */
#define TIOCM_CD        TIOCM_CAR   /* Carrier detect (synonym) */
#define TIOCM_RNG       0x080       /* Ring indicator */
#define TIOCM_RI        TIOCM_RNG   /* Ring indicator (synonym) */
#define TIOCM_DSR       0x100       /* Data set ready */

/**
 * @brief Terminal capability query structure
 */
typedef struct {
    char *name;                     /* Terminal name */
    uint32_t capabilities;          /* Capability flags */
    uint16_t max_colors;           /* Maximum number of colors */
    uint16_t max_pairs;            /* Maximum number of color pairs */
} TERM_CAPS;

/**
 * @brief Terminal capability flags
 */
#define TERM_CAP_COLOR      0x001   /* Color support */
#define TERM_CAP_CURSOR     0x002   /* Cursor positioning */
#define TERM_CAP_CLEAR      0x004   /* Screen clearing */
#define TERM_CAP_SCROLL     0x008   /* Scrolling support */
#define TERM_CAP_BOLD       0x010   /* Bold text */
#define TERM_CAP_UNDERLINE  0x020   /* Underlined text */
#define TERM_CAP_REVERSE    0x040   /* Reverse video */
#define TERM_CAP_BLINK      0x080   /* Blinking text */

/**
 * @brief Function prototypes for terminal control (if implementing in userspace)
 */
#ifdef __KERNEL__
/* Kernel-space function prototypes would go here */
#else
/* User-space function prototypes */
int tcgetattr(int fd, TERMIOS *termios_p);
int tcsetattr(int fd, int optional_actions, const TERMIOS *termios_p);
SPEED cfgetispeed(const TERMIOS *termios_p);
SPEED cfgetospeed(const TERMIOS *termios_p);
int cfsetispeed(TERMIOS *termios_p, SPEED speed);
int cfsetospeed(TERMIOS *termios_p, SPEED speed);
int cfsetspeed(TERMIOS *termios_p, SPEED speed);
int tcdrain(int fd);
int tcflow(int fd, int action);
int tcflush(int fd, int queue_selector);
int tcsendbreak(int fd, int duration);
#endif /* __KERNEL__ */