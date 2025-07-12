/**
 * @file termios_str.h
 * @author Zack Bostock
 * @brief Structures for a general terminal interface 
 * @ref https://pubs.opengroup.org/onlinepubs/7908799/xsh/termios.h.html
 * @ref https://www.man7.org/linux/man-pages/man3/termios.3.html
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <stdint.h>

/**
 * @brief Linux specific extensions
 */
#define TCGETS          0x5401
#define TCSETS          0x5402
#define TIOCGPGRP       0x540F
#define TIOCSPGRP       0x5410
#define TIOCGWINSZ      0x5413
#define TIOCSWINSZ      0x5414
#define TIOCGSID        0x5429

/**
 * @brief These are differnet local modes: Bitwise constants for c_lflag
 */
#define ECHO            0x0001  /* Enable echo */
#define ECHOE           0x0002  /* Echo erase character as error-correcting backspace */
#define ECHOK           0x0004  /* Echo kill */
#define ECHONL          0x0008  /* Echo NL */
#define ICANON          0x0010  /* Canonical input (erase and kill processing) */
#define IEXTEN          0x0020  /* Enable extended input character processing */
#define ISIG            0x0040  /* Enable signals */
#define NOFLSH          0x0080  /* Disable flash after interrupt or quit */
#define TOSTOP          0x0100  /* Send SIGTTOU for background output */

/**
 * @brief Indices for the c_cc array
 */
#define NCCS            11
#define VEOF            0
#define VEOL            1
#define VERASE          2
#define VINTR           3
#define VKILL           4
#define VMIN            5
#define VQUIT           6
#define VSTART          7
#define VSTOP           8
#define VSUSP           9
#define VTIME           10

typedef uint32_t CC;    /* Used for terminal space characters */
typedef uint32_t SPEED; /* used for terminal baud rates */
typedef uint32_t TCFLAG;/* Used for terminal modes */

/**
 * @brief Main termios structure
 */
typedef struct {
    TCFLAG c_iflag;     /* input modes */
    TCFLAG c_oflag;     /* output modes */
    TCFLAG c_cflag;     /* control modes */
    TCFLAG c_lflag;     /* local modes */
    CC c_cc[NCCS];      /* special characters */
    SPEED ibaud;
    SPEED obaud;
} TERMIOS;