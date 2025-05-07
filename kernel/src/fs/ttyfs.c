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

LOCK tty_lock;

VFS_FS ttyfs = {
    .name = "ttyfs",
    .is_temp = TRUE,
    .files = {0},
    .open = ttyfs_open,
    .mount = ttyfs_mount,
    .mknode = ttyfs_mknode,
    .rmnode = NULL,
    .sync = ttyfs_sync,
    .refresh = ttyfs_refresh,
    .read = ttyfs_read,
    .getdent = ttyfs_getdent,
    .write = ttyfs_write,
    .ioctl = ttyfs_ioctl
};

/**
 * @brief Create a ident for ttyfs
 *
 * @return TTYFS_IDENT* New ident
 */
static TTYFS_IDENT *create_ident() {
    TTYFS_IDENT *id = (TTYFS_IDENT *)(kmalloc(sizeof(TTYFS_IDENT)));
    if (!id) {
        kloge("TTYFS IDENT: Failed to allocate memory for ident in ttyfs\n");
        halt();
    }

    id->ibegin = 0;
    id->icursor = 0;
    id->isize = 0;

    memset(id->ibuff, 0, TTY_BUFFER_SIZE);
    memset(&(id->termios), 0, sizeof(TERMIOS));

    /* Enable signals, Canonical input, Enable echo */
    id->termios.c_lflag = (ISIG | ICANON | ECHO);

    /* Set the INTR character to 0x03 */
    /*
        NOTE: since signals are enabled, generate the corresponding INTR signal
              when receiving the INTR character
    */
    id->termios.c_cc[VINTR] = 0x03;

    return id;
}

/**
 * @brief Does nothing in current form
 */
void init_ttyfs() {
    klogs("INIT TTYFS: starting...\n");
    /* Nothing */
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
    TTYFS_IDENT *id = this->ident;
    int64_t ret = -1;
    LOCK_LOCK(&tty_lock);

    if (request == TIOCGWINSZ) {
        /* Get the window size */
        WINDOW_SIZE *ws = (WINDOW_SIZE *)(arg);
        terminal_get_winsize(ws);
        klogi("TTYFS IOCTL: TIOCGWINSZ returns row %d and col %d\n",
              ws->row, ws->col);
        ret = 0;
    } else if (request == TIOCSWINSZ) {
        /* Set the window size */
        WINDOW_SIZE *ws = (WINDOW_SIZE *)(arg);
        if (terminal_set_winsize(ws)) {
            ret = 0;
        }
    } else if (request == TIOCGPGRP) {
        /* Gets current process group */
        /* Do nothing */
    } else if (request == TIOCSPGRP) {
        /* Sets current process group */
        /* Do nothing */
    } else if (request == TCGETS) {
        TERMIOS *t = (TERMIOS *)(arg);
        *t = id->termios;
    } else if (request == TCSETS) {
        TERMIOS *t = (TERMIOS *)(arg);
        id->termios = *t;
    }

    UNLOCK_LOCK(&tty_lock);

    if (ret < 0) {
        cpu_set_errno(EINVAL);
    }

    return ret;
}


/**
 * @brief Main opening function for ttyfs
 *
 * @param this inode to open
 * @param path Path to inode
 * @return VFS_TNODE* Opened tnode
 */
VFS_TNODE *ttyfs_open(VFS_INODE *this, const char *path) {
    (void) this;

    return vfs_path_to_node(path, CREATE, VFS_DIRECTORY);

}

/**
 * @brief Main reading function for ttyfs
 *
 * @param this inode to read
 * @param offset Offset to start reading from
 * @param len Length to read
 * @param buff Buffer to copy into
 * @return int64_t Number of bytes read, or -1 if failure
 */
int64_t ttyfs_read(VFS_INODE *this, size_t offset, size_t len, void *buff) {
    TTYFS_IDENT *id = this->ident;

    LOCK_LOCK(&tty_lock);

    (void) offset;

    /* If we read less than len bytes, wait until there is enough data */
    while (id->isize < (int64_t) len) {
        uint64_t para = 0;
        UNLOCK_LOCK(&tty_lock);
        /* If this lock if locked (a.k.a it's waiting), we need to release it */
        UNLOCK_LOCK(&vfs_lock);
        if (cb_subscribe(sched_get_pid(), CB_KEY_PRESS, &para)) {
            LOCK_LOCK(&tty_lock);

            /* Maximally backtrace half of TTY_BUFFER_SIZE to determine whether */
            /* the backspace key should be accepted or not */
            int64_t dlen = 0;
            int64_t iend = (id->icursor + id->isize) % TTY_BUFFER_SIZE;
            id->ibegin = MAX(id->ibegin, id->icursor - TTY_BUFFER_SIZE / 2) %
                         TTY_BUFFER_SIZE;
            for (int64_t k = id->ibegin;; k++) {
                int64_t i = (id->ibegin + k) % TTY_BUFFER_SIZE;
                if (i == iend) {
                    break;
                }
                dlen += ((id->ibuff[i] != '\b') ? 1 : -1);
            }

            uint8_t keycode = para & 0xFF;
            if ((keycode && keycode != '\b') || (keycode == '\b' && dlen > 0)) {
                id->ibuff[iend] = keycode;
                id->isize += 1;
                if (id->isize >= TTY_BUFFER_SIZE) {
                    kloge("TTYFS READ: input buffer overflow!\n");
                    halt();
                }
            }
        } else {
            LOCK_LOCK(&tty_lock);
        }
        LOCK_LOCK(&vfs_lock);
    }

    /* We've now read the required amount from len input parameter */
    /* now, read from input buffer */
    int64_t rlen = MIN((int64_t)len, id->isize);

    for (int64_t i = 0; i < rlen; i++) {
        int64_t index = (id->icursor + i) % TTY_BUFFER_SIZE;
        ((char *) buff)[i] = id->ibuff[i];

        cursor_visible = TERM_CURSOR_HIDE;
        terminal_set_cursor(' ');
        terminal_refresh(TERM_MODE_TERM);

        if (id->ibuff[index] != (char)EOF) {
            kprintf("%c", id->ibuff[index]);
        } else {
            kprintf("[EOF]\n");
        }
        cursor_visible = TERM_CURSOR_INVISIBLE;
    }

    /* Update start and size of input buffer */
    id->icursor += rlen;
    id->icursor %= TTY_BUFFER_SIZE;
    id->isize -= rlen;

    UNLOCK_LOCK(&tty_lock);

    return rlen;
}

/**
 * @brief Main writing function for ttyfs
 *
 * @param this inode to write to
 * @param offset Offset to start writing from
 * @param len Length to write
 * @param buff Buffer to write from
 * @return int64_t Number of bytes written, or -1 if failure
 */
int64_t ttyfs_write(VFS_INODE *this, size_t offset, size_t len, const void *buff) {
    TTYFS_IDENT *id = this->ident;
    int64_t wlen = 0;

    (void) offset;

    /* Reset the input buffer */
    id->ibegin = 0;
    id->icursor = 0;
    id->isize = 0;

    /* Output to the terminal */
    char msg_buff[3] = {0};
    char *msg = (len > 1) ? (char *)(kmalloc(len + 1)) : msg_buff;
    if (msg) {
        msg[len] = '\0';
        memcpy(msg, buff, len);

        cursor_visible = TERM_CURSOR_HIDE;
        terminal_set_cursor(' ');
        terminal_refresh(TERM_MODE_TERM);

        kprintf("%s", msg);

        cursor_visible = TERM_CURSOR_INVISIBLE;

        if (len > 1) {
            kfree(msg);
        }

        wlen = len;
    }

    UNLOCK_LOCK(&tty_lock);
    return wlen;
}

/**
 * @brief Main sync function of ttyfs
 *
 * @param this inode to sync
 * @return int64_t 0 to success, -1 if failure
 */
int64_t ttyfs_sync(VFS_INODE *this) {
    (void) this;
    return 0;
}

/**
 * @brief Main refresh function of ttyfs
 *
 * @param this inode to refresh
 * @return int64_t 0 to success, -1 if failure
 */
int64_t ttyfs_refresh(VFS_INODE *this) {
    (void) this;
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
    (void) this;
    (void) pos;
    (void) dirent;
    return -1;
}

/**
 * @brief Main node making function for ttyfs
 *
 * @param this tnode to make a new inode at
 * @return int64_t 0 if success, -1 if failure
 */
int64_t ttyfs_mknode(VFS_TNODE *this) {
    this->inode->ident = create_ident();
    return 0;
}

/**
 * @brief Main mounting function for ttyfs
 *
 * @param at Location to mount at
 * @return VFS_INODE* New inode where ttyfs is mounted at
 */
VFS_INODE *ttyfs_mount(VFS_INODE *at) {
    klogi("TTYFS MOUNT: Mounted ttyfs at %x\n", at);
    VFS_INODE *ret = vfs_alloc_inode(VFS_MOUNT_POINT, 0777, 0, &ttyfs, NULL);
    ret->ident = create_ident();

    return ret;
}
