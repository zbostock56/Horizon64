/**
 * @file pipefs.c
 * @author Zack Bostock
 * @brief Functionality pertaining to piping filesystem
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <libc/errno.h>

#include <common/string.h>
#include <common/kprint.h>
#include <common/kmalloc.h>
#include <common/lock.h>

#include <proc/ctxsw.h>

#include <fs/pipefs.h>
#include <fs/vfs.h>

#include <sys/mmu.h>

static LOCK pipe_lock = {0};
extern LOCK vfs_lock;

VFS_FS pipefs = {
    .name = "pipefs",
    .is_temp = 1,
    .files = {0},
    .open = pipefs_open,
    .mount = pipefs_mount,
    .mknode = pipefs_mknode,
    .rmnode = pipefs_rmnode,
    .sync = NULL,
    .refresh = NULL,
    .read = pipefs_read,
    .getdent = NULL,
    .write = pipefs_write,
    .ioctl = NULL
};

static uint8_t pipe_eof_magic_word[5] = {0xFF, 0x0E, 0x00, 0x0F, 0x00};

/**
 * @brief Helper to create ident
 *
 * @return PIPEFS_IDENT* New ident
 */
static PIPEFS_IDENT *create_ident() {
    PIPEFS_IDENT *id = (PIPEFS_IDENT *)(kmalloc(sizeof(PIPEFS_IDENT)));
    if (!id) {
        kloge("PIPEFS CREATE IDENT: Failed to allocate memory!\n");
        halt();
    }

    memset(id, 0, sizeof(PIPEFS_IDENT));
    return id;
}

/**
 * @brief Stub function to ensure compatibility
 */
void init_pipefs() {
    klogi("INIT PIPEFS: starting...\n");
    /* Nothing */
    klogi("INIT PIPEFS: finished...\n");
}

/**
 * @brief Main open function for pipe filesystem
 *
 * @param this inode to open
 * @param path path to open
 * @return VFS_TNODE* opened tnode
 */
VFS_TNODE *pipefs_open(VFS_INODE *this, const char *path) {
    (void) this;
    return vfs_path_to_node(path, CREATE, VFS_CHAR_DEV);
}

/**
 * @brief Main read funciton for pipefs filesystem
 *
 * @param this inode to read from
 * @param offset Doesn't matter
 * @param len Number of bytes to read
 * @param buff Buffer to copy into
 * @return int64_t Number of bytes read
 */
int64_t pipefs_read(VFS_INODE *this, size_t offset, size_t len, void *buff) {
    PIPEFS_IDENT *id = this->ident;
    size_t rlen = 0;                    /* Read length */
    (void) offset;                      /* Not used here */

    if (id->size == 0 || id->closed) {
        return 0;
    }


    /* Block until the pipe starts to fill up */
    while (id->size == 0) {
        UNLOCK_LOCK(&vfs_lock);
        sched_sleep(0);
        LOCK_LOCK(&vfs_lock);
    }

    uint8_t tempbuff[4] = {0};
    LOCK_LOCK(&pipe_lock);

    rlen = id->size;
    if (rlen > len) {
        rlen = len;
    }

    if (rlen > 4) {
        memcpy(buff, id->buff, rlen - 4);
        memcpy(tempbuff, (uint8_t *)id->buff + rlen - 4, 4);
    } else {
        memcpy(buff, id->buff, rlen);
    }

    if (id->size - rlen > 0) {
        char val = ((char *)id->buff)[id->size - 1];
        memcpy(id->buff, &(id->buff[rlen]), id->size - rlen);
        if(((char *)id->buff)[id->size - rlen - 1] != val) {
            kloge("PIPEFS READ: Corruption in memcpy() whil reading %d bytes "
                  "from %d byte-sized buffer!\n", rlen, id->size);
            halt();
        }
    }

    /* Remove the number of bytes read */
    id->size -= rlen;

    /* Double check buffer magic */
    if (rlen >= 4) {
        uint8_t need_copy = TRUE;
        if (tempbuff[0] == pipe_eof_magic_word[0] &&
            tempbuff[1] == pipe_eof_magic_word[1] &&
            tempbuff[2] == pipe_eof_magic_word[2] &&
            tempbuff[3] == pipe_eof_magic_word[3]) {
            id->closed = TRUE;
            need_copy = FALSE;
            rlen -= 4;
        }
        if (need_copy) {
            memcpy(buff + rlen - 4, tempbuff, 4);
        }
    }

    UNLOCK_LOCK(&pipe_lock);

    klogd("PIPEFS READ: read %d bytes to %x and return %d bytes (%02x)\n",
           len, buff, rlen, (rlen > 0 ? ((char *)buff)[rlen - 1] : 0));

    return rlen;
}

/**
 * @brief Main writing function for pipefs
 *
 * @param this inode to manipulate
 * @param offset Not used
 * @param len Length of buffer to write
 * @param buff Buffer with data to write into pipe
 * @return int64_t Number of bytes written
 */
int64_t pipefs_write(VFS_INODE *this, size_t offset, size_t len, const void *buff) {
    PIPEFS_IDENT *id = this->ident;
    size_t wlen = 0;                    /* Write length */
    (void) offset;                      /* Not used here */

    klogd("PIPEFS WRITE: write %d bytes from %x to %x whose size is "
          "%d bytes and %d refcount\n", len, buff, id->buff, id->size,
          this->references);

    LOCK_LOCK(&pipe_lock);

    /* Wait until there is room available in the pipe */
    while (TRUE) {
        if (PIPE_BUFFER_SIZE > id->size) {
            wlen = PIPE_BUFFER_SIZE - id->size;
        }
        if (wlen > 0) {
            break;
        }

        UNLOCK_LOCK(&vfs_lock);
        sched_sleep(0);
        LOCK_LOCK(&vfs_lock);
    }

    if (wlen > len) {
        wlen = len;
    }
    memcpy(&(id->buff[id->size]), buff, wlen);

    /* Increase the size of the buffer with new bytes written into it */
    id->size += wlen;

    UNLOCK_LOCK(&pipe_lock);

    return wlen;
}

/**
 * @brief Main node creation function for pipefs
 *
 * @param this tnode to make into pipe
 * @return int64_t 0 if success, -1 if failure
 */
int64_t pipefs_mknode(VFS_TNODE *this) {
    this->inode->ident = create_ident();
    return 0;
}

/**
 * @brief Main node removal function for pipefs
 *
 * @param this Node to remove
 * @return int64_t 0 if success, -1 if error
 */
int64_t pipefs_rmnode(VFS_TNODE *this) {
    PIPEFS_IDENT *id = (PIPEFS_IDENT *)(this->inode->ident);

    if (!id) {
        return -1;
    }

    /* Remove the node */
    kfree(id);

    /* Remove this node from it's parent */
    VFS_INODE *parent = this->parent;
    for (size_t i = 0; i < vector_len(&parent->child); i++) {
        VFS_TNODE *t = vector_at(&parent->child, i);
        if (t == this) {
            vector_erase(&parent->child, i);
            return 0;
        }
    }

    return -1;
}

/**
 * @brief Main mounting function for pipefs file system
 *
 * @param at Location to mount at
 * @return VFS_INODE* Newly mounted pipefs
 */
VFS_INODE *pipefs_mount(VFS_INODE *at) {
    (void) at;

    klogi("PIPEFS MOUNT: mounting pipefs filesystem...\n");
    VFS_INODE *ret = vfs_alloc_inode(VFS_MOUNT_POINT, 0777, 0, &pipefs, NULL);
    ret->ident = create_ident();
    return ret;
}