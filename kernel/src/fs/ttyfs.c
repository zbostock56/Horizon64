/**
 * @file ttyfs.c
 * @author Zack Bostock
 * @brief Functionality pertaining to ttyfs
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <libc/errno.h>
#include <libc/stdio.h>

#include <common/string.h>
#include <common/kmalloc.h>
#include <common/kprint.h>
#include <common/lock.h>
#include <common/math.h>

#include <dev/terminal.h>

#include <sys/asm.h>
#include <sys/cpu.h>
#include <sys/smp.h>

#include <fs/ttyfs.h>
#include <fs/vfs.h>

#include <proc/callback.h>
#include <proc/ctxsw.h>

extern LOCK vfs_lock;

LOCK tty_lock = LOCK_NEW;

VFS_FS ttyfs = {
    .name = "ttyfs",
    .is_temp = TRUE,
    .files = {0},
    .open = ttyfs_open,
    .mount = ttyfs_mount,
    .mknode = ttyfs_mknode,
    .rmnode = ttyfs_rmnode,
    .sync = ttyfs_sync,
    .refresh = ttyfs_refresh,
    .read = ttyfs_read,
    .getdent = ttyfs_getdent,
    .write = ttyfs_write,
    .ioctl = ttyfs_ioctl
};

/**
 * @brief Create and initialize a TTY identifier structure
 *
 * @return TTYFS_IDENT* New identifier, or NULL on failure
 */
static TTYFS_IDENT *create_ident(void) {
    TTYFS_IDENT *id = (TTYFS_IDENT *)kmalloc(sizeof(TTYFS_IDENT));
    if (!id) {
        kloge("TTYFS IDENT: Failed to allocate memory for ident\n");
        cpu_set_errno(ENOMEM);
        return NULL;
    }

    /* Set up buffer state */
    id->ibegin = 0;
    id->icursor = 0;
    id->isize = 0;
    id->line_ready = FALSE;
    id->eof_received = FALSE;

    /* Clear all buffers */
    memset(id->ibuff, 0, TTY_BUFFER_SIZE);
    memset(&(id->termios), 0, sizeof(TERMIOS));

    /* Set default terminal attributes */
    /* Input flags: enable break processing */
    id->termios.c_iflag = BRKINT;

    /* Output flags: Enable post-processing */
    id->termios.c_oflag = OPOST;

    /* Control flags: 8-bit chars, enable receiver */
    id->termios.c_cflag = CS8 | CREAD;

    /* Local flags: Enable signals, canonical input, and echo characters */
    id->termios.c_lflag = (ISIG | ICANON | ECHO | ECHOE | ECHOK);

    /* Control characters */
    id->termios.c_cc[VINTR] = 0x03;    /* Ctrl-C */
    id->termios.c_cc[VQUIT] = 0x1C;    /* Ctrl-\ */
    id->termios.c_cc[VERASE] = 0x08;   /* Backspace */
    id->termios.c_cc[VKILL] = 0x15;    /* Ctrl-U */
    id->termios.c_cc[VEOF] = 0x04;     /* Ctrl-D */
    id->termios.c_cc[VEOL] = 0x00;     /* No end of line char */
    id->termios.c_cc[VMIN] = 1;        /* Minimum chars for non-canonical read */
    id->termios.c_cc[VTIME] = 0;       /* Timeout for non-canonical read */

    return id;
}

/**
 * @brief Safely destroy a TTY identifier structure
 *
 * @param id Identifier to destroy
 */
static void destroy_ident(TTYFS_IDENT *id) {
    if (id) {
        memset(id->ibuff, 0, TTY_BUFFER_SIZE);
        kfree(id);
    }
}

/**
 * @brief Validate TTY identifier structure
 *
 * @param id Identifier to validate
 * @return bool TRUE if valid, FALSE otherwise
 */
static bool validate_ident(TTYFS_IDENT *id) {
    if (!id) {
        return FALSE;
    }

    /* Check buffer indices are within bounds */
    if (id->ibegin >= TTY_BUFFER_SIZE ||
        id->icursor >= TTY_BUFFER_SIZE ||
        id->isize < 0 || id->isize > TTY_BUFFER_SIZE) {
        kloge("TTYFS: Invalid buffer state detected\n");
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Safely write data to the input buffer
 *
 * @param id TTY identifier
 * @param data Data to write
 * @return int64_t 0 on success, -1 on failure
 */
static int64_t safe_buffer_write(TTYFS_IDENT *id, uint8_t data) {
    if (!validate_ident(id)) {
        return -1;
    }

    if (id->isize >= TTY_BUFFER_SIZE - 1) {
        kloge("TTYFS: Input buffer full, dropping character\n");
        return -1;
    }

    int64_t write_pos = (id->icursor + id->isize) % TTY_BUFFER_SIZE;
    id->ibuff[write_pos] = data;
    id->isize++;

    return 0;
}

/**
 * @brief Reset the input buffer to empty state
 *
 * @param id TTY identifier
 */
static void reset_input_buffer(TTYFS_IDENT *id) {
    if (!id) return;

    id->ibegin = 0;
    id->icursor = 0;
    id->isize = 0;
    id->line_ready = FALSE;
    memset(id->ibuff, 0, TTY_BUFFER_SIZE);
}

/**
 * @brief Calculate the display length of current input line
 *
 * @param id TTY identifier
 * @return int64_t Display length
 */
static int64_t calculate_display_length(TTYFS_IDENT *id) {
    if (!validate_ident(id)) {
        return 0;
    }

    int64_t display_len = 0;
    int64_t end_pos = (id->icursor + id->isize) % TTY_BUFFER_SIZE;

    for (int64_t i = id->ibegin; i != end_pos; i = (i + 1) % TTY_BUFFER_SIZE) {
        if (id->ibuff[i] == '\b') {
            display_len = MAX(0, display_len - 1);
        } else if (id->ibuff[i] >= 0x20 || id->ibuff[i] == '\t') {
            display_len++;
        }
    }

    return display_len;
}

/**
 * @brief Handle special control characters
 *
 * @param id TTY identifier
 * @param keycode Character to handle
 */
static void handle_special_chars(TTYFS_IDENT *id, uint8_t keycode) {
    if (!id) return;

    /* Handle interrupt signal (Ctrl-C) */
    if (keycode == id->termios.c_cc[VINTR] && (id->termios.c_lflag & ISIG)) {
        /* Send SIGINT to process group */
        klogi("TTYFS: SIGINT received\n");
        reset_input_buffer(id);
        return;
    }

    /* Handle quit signal (Ctrl-\) */
    if (keycode == id->termios.c_cc[VQUIT] && (id->termios.c_lflag & ISIG)) {
        /* Send SIGQUIT to process group */
        klogi("TTYFS: SIGQUIT received\n");
        klogw("TTYFS: Process groups are not yet supported\n");
        reset_input_buffer(id);
        return;
    }

    /* Handle EOF (Ctrl-D) */
    if (keycode == id->termios.c_cc[VEOF]) {
        id->eof_received = TRUE;
        id->line_ready = TRUE;
        return;
    }

    /* Handle line kill (Ctrl-U) */
    if (keycode == id->termios.c_cc[VKILL]) {
        if (id->termios.c_lflag & ECHOK) {
            /* Echo kill by showing ^U and clearing line */
            kprintf("^U\n");
        }
        reset_input_buffer(id);
        return;
    }
}

/**
 * @brief Process input in canonical mode
 *
 * @param id TTY identifier
 * @param keycode Character to process
 * @return int64_t 0 on success, -1 on failure
 */
static int64_t process_canonical_input(TTYFS_IDENT *id, uint8_t keycode) {
    if (!validate_ident(id)) {
        return -1;
    }

    /* Handle special characters first */
    handle_special_chars(id, keycode);
    if (id->line_ready) {
        return 0;
    }

    /* Handle backspace character */
    if (keycode == id->termios.c_cc[VERASE] || keycode == '\b') {
        int64_t display_len = calculate_display_length(id);
        if (display_len > 0 && id->isize > 0) {
            if (safe_buffer_write(id, '\b') == 0) {
                if (id->termios.c_lflag & ECHOE) {
                    kprintf("\b \b");  // Erase character visually
                }
            }
        }
        return 0;
    }

    /* Handle end of line */
    if (keycode == '\n' || keycode == '\r' || keycode == id->termios.c_cc[VEOL]) {
        if (safe_buffer_write(id, '\n') == 0) {
            id->line_ready = TRUE;
            if (id->termios.c_lflag & ECHO) {
                kprintf("\n");
            }
        }
        return 0;
    }

    /* Handle regular characters */
    if (keycode >= 0x20 || keycode == '\t') {
        if (safe_buffer_write(id, keycode) == 0) {
            if (id->termios.c_lflag & ECHO) {
                kprintf("%c", keycode);
            }
        }
        return 0;
    }

    return -1;
}

/**
 * @brief Process input in raw mode
 *
 * @param id TTY identifier
 * @param keycode Character to process
 * @return int64_t 0 on success, -1 on failure
 */
static int64_t process_raw_input(TTYFS_IDENT *id, uint8_t keycode) {
    if (!validate_ident(id)) {
        return -1;
    }

    return safe_buffer_write(id, keycode);
}

/**
 * @brief Initialize TTY filesystem
 */
void init_ttyfs(void) {
    klogs("INIT TTYFS: starting...\n");
    /* Nothing to do */
    klogs("INIT TTYFS: finished...\n");
}

/**
 * @brief Main I/O control function for ttyfs
 *
 * @param this inode to control
 * @param request Action to perform
 * @param arg Arguments that go along with the action/request
 * @return int64_t 0 if success, -1 if failure
 */
int64_t ttyfs_ioctl(VFS_INODE *this, int64_t request, int64_t arg) {
    if (!this || !this->ident) {
        cpu_set_errno(EINVAL);
        return -1;
    }

    TTYFS_IDENT *id = this->ident;
    int64_t ret = -1;

    LOCK_LOCK(&tty_lock);

    switch (request) {
        case TIOCGWINSZ: {
            /* Get the window size */
            WINDOW_SIZE *ws = (WINDOW_SIZE *)arg;
            if (ws && terminal_get_winsize(ws) == SYS_OK) {
                klogi("TTYFS IOCTL: TIOCGWINSZ returns row %d and col %d\n",
                      ws->row, ws->col);
                ret = 0;
            }
            break;
        }

        case TIOCSWINSZ: {
            /* Set the window size */
            WINDOW_SIZE *ws = (WINDOW_SIZE *)arg;
            if (ws && terminal_set_winsize(ws) == 0) {
                ret = 0;
            }
            break;
        }

        case TIOCGPGRP: {
            /* Gets current process group */
            klogw("TTYFS: Does not currently support getting process group\n");
            break;
        }

        case TIOCSPGRP: {
            /* Sets current process group */
            klogw("TTYFS: Does not currently support setting process group\n");
            break;
        }

        case TCGETS: {
            TERMIOS *t = (TERMIOS *)arg;
            if (t) {
                *t = id->termios;
                ret = 0;
            }
            break;
        }

        case TCSETS:
        case TCSETSW:
        case TCSETSF: {
            TERMIOS *t = (TERMIOS *)arg;
            if (t) {
                if (request == TCSETSF) {
                    // Flush input buffer
                    reset_input_buffer(id);
                }
                id->termios = *t;
                ret = 0;
            }
            break;
        }

        case TCFLSH: {
            int queue = (int)arg;
            switch (queue) {
                case TCIFLUSH:
                case TCIOFLUSH:
                    reset_input_buffer(id);
                    ret = 0;
                    break;
                case TCOFLUSH:
                    // Flush output - implementation depends on your output buffering
                    ret = 0;
                    break;
                default:
                    cpu_set_errno(EINVAL);
                    break;
            }
            break;
        }

        default:
            cpu_set_errno(ENOTTY);
            break;
    }

    UNLOCK_LOCK(&tty_lock);

    if (ret < 0 && cpu_get_errno() == 0) {
        cpu_set_errno(EINVAL);
    }

    return ret;
}

/**
 * @brief Main opening function for ttyfs
 *
 * @param this inode to open
 * @param path Path to inode
 * @return VFS_TNODE* Opened tnode, or NULL on failure
 */
VFS_TNODE *ttyfs_open(VFS_INODE *this, const char *path) {
    (void)this;
    if (!path) {
        cpu_set_errno(EINVAL);
        return NULL;
    }

    VFS_TNODE *node = vfs_path_to_node(path, CREATE, VFS_DIRECTORY);
    if (!node) {
        kloge("TTYFS OPEN: Failed to create/find node for path: %s\n", path);
        cpu_set_errno(ENOENT);
    }

    return node;
}

/**
 * @brief Main reading function for ttyfs
 *
 * @param this inode to read
 * @param offset Offset to start reading from (ignored for TTY)
 * @param len Length to read
 * @param buff Buffer to copy into
 * @return int64_t Number of bytes read, or -1 if failure
 */
int64_t ttyfs_read(VFS_INODE *this, size_t offset, size_t len, void *buff) {
    if (!this || !this->ident || !buff || len == 0) {
        cpu_set_errno(EINVAL);
        return -1;
    }

    TTYFS_IDENT *id = this->ident;

    /* TTY doesn't support seeking */
    (void)offset;

    LOCK_LOCK(&tty_lock);

    /* In canonical mode, wait for a complete line */
    if (id->termios.c_lflag & ICANON) {
        while (!id->line_ready && !id->eof_received) {
            uint64_t para = 0;
            UNLOCK_LOCK(&tty_lock);
            UNLOCK_LOCK(&vfs_lock);

            if (cb_subscribe(sched_get_pid(), CB_KEY_PRESS, &para)) {
                LOCK_LOCK(&tty_lock);
                uint8_t keycode = para & 0xFF;
                if (keycode) {
                    process_canonical_input(id, keycode);
                }
            } else {
                LOCK_LOCK(&tty_lock);
            }

            LOCK_LOCK(&vfs_lock);
        }
    } else {
        /* Raw mode - read available characters up to VMIN */
        size_t min_chars = id->termios.c_cc[VMIN];
        if (min_chars == 0) min_chars = 1;

        while ((size_t)id->isize < (size_t)MIN(len, min_chars)) {
            uint64_t para = 0;
            UNLOCK_LOCK(&tty_lock);
            UNLOCK_LOCK(&vfs_lock);

            if (cb_subscribe(sched_get_pid(), CB_KEY_PRESS, &para)) {
                LOCK_LOCK(&tty_lock);
                uint8_t keycode = para & 0xFF;
                if (keycode) {
                    process_raw_input(id, keycode);
                }
            } else {
                LOCK_LOCK(&tty_lock);
            }

            LOCK_LOCK(&vfs_lock);
        }
    }

    /* Handle EOF condition */
    if (id->eof_received && id->isize == 0) {
        id->eof_received = FALSE;
        UNLOCK_LOCK(&tty_lock);
        /* EOF */
        return 0;
    }

    /* Copy data to user buffer */
    int64_t bytes_to_read = MIN((int64_t)len, id->isize);
    char *output = (char *)buff;

    for (int64_t i = 0; i < bytes_to_read; i++) {
        int64_t index = (id->icursor + i) % TTY_BUFFER_SIZE;
        output[i] = id->ibuff[index];
    }

    /* Update buffer state */
    id->icursor = (id->icursor + bytes_to_read) % TTY_BUFFER_SIZE;
    id->isize -= bytes_to_read;

    /* Reset line ready flag, if we've consumed the line */
    if (id->isize == 0) {
        id->line_ready = FALSE;
    }

    UNLOCK_LOCK(&tty_lock);
    return bytes_to_read;
}

/**
 * @brief Main writing function for ttyfs
 *
 * @param this inode to write to
 * @param offset Offset to start writing from (ignored for TTY)
 * @param len Length to write
 * @param buff Buffer to write from
 * @return int64_t Number of bytes written, or -1 if failure
 */
int64_t ttyfs_write(VFS_INODE *this, size_t offset, size_t len, const void *buff) {
    if (!this || !this->ident || !buff || len == 0) {
        cpu_set_errno(EINVAL);
        return -1;
    }

    TTYFS_IDENT *id = this->ident;

    /* TTY does not support seeking */
    (void)offset;

    LOCK_LOCK(&tty_lock);

    /* Allocate message buffer */
    char *msg = NULL;

    /* Use stack for small messages */
    char stack_buff[256];

    if (len < sizeof(stack_buff)) {
        msg = stack_buff;
    } else {
        msg = (char *)kmalloc(len + 1);
        if (!msg) {
            UNLOCK_LOCK(&tty_lock);
            cpu_set_errno(ENOMEM);
            return -1;
        }
    }

    memcpy(msg, buff, len);
    msg[len] = '\0';

    /* Output to terminal */
    cursor_visible = TERM_CURSOR_HIDE;
    terminal_set_cursor(' ');
    terminal_refresh(TERM_MODE_TERM);

    /* Process output according to termios flags */
    if (id->termios.c_oflag & OPOST) {
        /* Post-process output */
        for (size_t i = 0; i < len; i++) {
            char c = msg[i];
            if (c == '\n' && (id->termios.c_oflag & ONLCR)) {
                /* Convert LF to CRLF */
                kprintf("\r\n");
            } else {
                kprintf("%c", c);
            }
        }
    } else {
        /* Raw output */
        kprintf("%s", msg);
    }

    cursor_visible = TERM_CURSOR_INVISIBLE;

    /* Free allocated memory if we used heap */
    if (msg != stack_buff) {
        kfree(msg);
    }

    UNLOCK_LOCK(&tty_lock);
    return (int64_t)len;
}

/**
 * @brief Main sync function of ttyfs
 *
 * @param this inode to sync
 * @return int64_t 0 on success, -1 if failure
 */
int64_t ttyfs_sync(VFS_INODE *this) {
    if (!this) {
        cpu_set_errno(EINVAL);
        return -1;
    }

    /* TTY doesn't need syncing, but we can flush terminal */
    terminal_refresh(TERM_MODE_TERM);
    return 0;
}

/**
 * @brief Main refresh function of ttyfs
 *
 * @param this inode to refresh
 * @return int64_t 0 on success, -1 if failure
 */
int64_t ttyfs_refresh(VFS_INODE *this) {
    if (!this) {
        cpu_set_errno(EINVAL);
        return -1;
    }

    terminal_refresh(TERM_MODE_TERM);
    return 0;
}

/**
 * @brief Main getting directory entry function for ttyfs
 *
 * @param this inode to get dirent of
 * @param pos Position
 * @param dirent Directory entry
 * @return int64_t 0 if success, -1 if failure
 */
int64_t ttyfs_getdent(VFS_INODE *this, size_t pos, VFS_DIR_ENTRY *dirent) {
    (void)this;
    (void)pos;
    (void)dirent;

    /* TTY devices don't support directory operations */
    cpu_set_errno(ENOTDIR);
    return -1;
}

/**
 * @brief Main node making function for ttyfs
 *
 * @param this tnode to make a new inode at
 * @return int64_t 0 if success, -1 if failure
 */
int64_t ttyfs_mknode(VFS_TNODE *this) {
    if (!this || !this->inode) {
        cpu_set_errno(EINVAL);
        return -1;
    }

    this->inode->ident = create_ident();
    if (!this->inode->ident) {
        return -1;
    }

    return 0;
}

/**
 * @brief Remove a TTY node
 *
 * @param this tnode to remove
 * @return int64_t 0 if success, -1 if failure
 */
int64_t ttyfs_rmnode(VFS_TNODE *this) {
    if (!this || !this->inode) {
        cpu_set_errno(EINVAL);
        return -1;
    }

    if (this->inode->ident) {
        destroy_ident((TTYFS_IDENT *)this->inode->ident);
        this->inode->ident = NULL;
    }

    return 0;
}

/**
 * @brief Main mounting function for ttyfs
 *
 * @param at Location to mount at
 * @return VFS_INODE* New inode where ttyfs is mounted at, or NULL on failure
 */
VFS_INODE *ttyfs_mount(VFS_INODE *at) {
    if (!at) {
        cpu_set_errno(EINVAL);
        return NULL;
    }

    klogi("TTYFS MOUNT: Mounting ttyfs at %x\n", at);

    VFS_INODE *ret = vfs_alloc_inode(VFS_MOUNT_POINT, 0666, 0, &ttyfs, NULL);
    if (!ret) {
        kloge("TTYFS MOUNT: Failed to allocate inode\n");
        cpu_set_errno(ENOMEM);
        return NULL;
    }

    ret->ident = create_ident();
    if (!ret->ident) {
        /* create_ident() would have already set errno */
        kfree(ret);
        return NULL;
    }

    return ret;
}