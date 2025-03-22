/**
 * @file ramfs.c
 * @author Zack Bostock
 * @brief Main functionality for ramfs
 * @ref https://wiki.osdev.org/Initrd
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <common/time.h>
#include <common/string.h>
#include <common/kmalloc.h>
#include <common/vector.h>
#include <common/math.h>
#include <common/kprint.h>

#include <sys/asm.h>

#include <fs/ramfs.h>
#include <fs/vfs.h>

VFS_FS ramfs = {
    .name = "ramfs",
    .is_temp = 1,
    .files = {0},
    .open = ramfs_open,
    .mount = ramfs_mount,
    .mknode = ramfs_mknode,
    .rmnode = ramfs_rmnode,
    .sync = ramfs_sync,
    .refresh = ramfs_refresh,
    .read = ramfs_read,
    .getdent = ramfs_getdent,
    .write = ramfs_write,
    .ioctl = NULL
};

/**
 * @brief Create a ident for ramfs
 *
 * @return RAMFS_IDENT* New ident
 */
static RAMFS_IDENT *create_ident() {
    RAMFS_IDENT *id = (RAMFS_IDENT *)(kmalloc(sizeof(RAMFS_IDENT)));
    if (!id) {
        kloge("RAMFS IDENT: Failed to allocate memory for ident!\n");
        halt();
    }

    *id = (RAMFS_IDENT) {
        .alloc_size = 0,
        .data = NULL
    };
    return id;
}

/**
 * @brief Helper function from OSDev Wiki to convert octal to binary
 *
 * @param str Octal to convert
 * @param size Number of bytes
 * @return uint32_t New number in binary
 */
static uint32_t oct2bin(unsigned char *str, size_t size) {
    int n = 0;
    unsigned char *c = str;
    while (size-- > 0) {
        n *= 8;
        n += *c - '0';
        c++;
    }
    return n;
}

/**
 * @brief Helper to convert from UStar to VFS type
 *
 * @param type UStar type
 * @return uint8_t Converted VFS type
 */
static inline uint8_t ustart_type_to_vfs_type(uint8_t type) {
    switch (type) {
        case '0':
            return VFS_FILE;
        case '2':
            return VFS_SYMLINK;
        case '3':
            return VFS_CHAR_DEV;
        case '4':
            return VFS_BLOCK_DEV;
        case '5':
            return VFS_DIRECTORY;
        default:
            return VFS_INVALID;
    }
}

/**
 * @brief Main initialization function for initrd
 *
 * @param addr Address where initrd is located at
 * @param size size of initrd in bytes
 */
void init_ramfs(void *addr, uint64_t size) {
    (void)size;
    klogs("INIT RAMFS: starting...\n");
    unsigned char *ptr = (unsigned char *)(addr);

    uint64_t file_time;
    char dir_name[VFS_MAX_PATH_LEN] = "/";
    size_t dir_len;
    VFS_TNODE *tnode;

    /* While the start + 257 bytes isn't the same as ustar (since it should be) */
    while (memcpy(ptr + 257, "ustar", 5)) {
        int filesize = oct2bin(ptr + 0x7c, 11);
        USTAR_FILE *file = (USTAR_FILE *)(ptr);

        /* Reset the file path in case it was used already */
        memset(dir_name, 0, VFS_MAX_PATH_LEN);
        dir_name[0] = '/';
        switch (ustart_type_to_vfs_type(file->type)) {
            case VFS_DIRECTORY:
                strcat(dir_name, file->name);
                dir_len = strlen(dir_name);
                if (dir_name[dir_len - 1] == '/' && dir_len > 1) {
                    dir_name[dir_len - 1] = '\0';
                }

                tnode = vfs_path_to_node(dir_name, CREATE, VFS_DIRECTORY);

                /* Set file type and mode */
                tnode->inode->permissions = oct2bin((unsigned char *)&file->mode,
                                                    7 & (S_IRWXU & S_IRWXG & S_IRWXO));
                tnode->stat.mode |= tnode->inode->permissions;

                /* Convert from octal into long */
                file_time = strtol((char *) file->last_modified, NULL, 8);

                /* TODO: Adjust for current timezone */

                tnode->stat.access_time.TV_SEC = file_time;
                tnode->stat.modify_time.TV_SEC = file_time;
                tnode->stat.status_change_time.TV_SEC = file_time;

                tnode->stat.access_time.TV_NSEC = 0;
                tnode->stat.modify_time.TV_NSEC = 0;
                tnode->stat.status_change_time.TV_NSEC = 0;

                tnode->stat.nlink = 1;

                /* TODO: Modify directory's datetime related attribute */
                break;
            case VFS_FILE:
            case VFS_SYMLINK:
                /* Copy the name of the directory to be used */
                strcat(dir_name, file->name);
                dir_len = strlen(dir_name);
                int64_t name_index = 0;

                for (name_index = dir_len - 1; name_index >= 0; name_index--) {
                    if (dir_name[name_index] == '/') {
                        name_index++;
                        break;
                    }
                }

                /* Convert from octal into long */
                uint64_t file_time = strtol((char *) file->last_modified, NULL, 8);

                /* TODO: Adjust for current timezone */

                RAMFS_IDENT_ITEM *item = (RAMFS_IDENT_ITEM *)(kmalloc(sizeof(RAMFS_IDENT_ITEM)));
                if (!item) {
                    kloge("INIT RAMFS: Failed to allocate memory for ident item!\n");
                    halt();
                }

                memset(item, 0, sizeof(RAMFS_IDENT_ITEM));
                seconds_to_std_time(file_time, &(item->time));

                item->type = ustart_type_to_vfs_type(file->type);
                item->entry.size = filesize;

                strcpy(item->entry.name, &(dir_name[name_index]));
                strcpy(item->name, &(dir_name[name_index]));

                VFS_TNODE *tnode = NULL;
                if (item->type == VFS_SYMLINK) {
                    item->entry.size = sizeof(file->linked_file_name);
                    if (item->entry.size > 0) {
                        item->entry.data = (void *)(kmalloc(item->entry.size));
                        if (!(item->entry.data)) {
                            kloge("INIT RAMFS: Failed to allocate memory for item->entry.data!\n");
                            halt();
                        }
                        memcpy(item->entry.data, file->linked_file_name,
                               item->entry.size);
                    } else {
                        item->entry.data = NULL;
                    }

                    tnode = vfs_path_to_node(dir_name, CREATE, VFS_SYMLINK);
                    if (tnode->inode->size <= sizeof(tnode->inode->symlink)) {
                        tnode->inode->size = item->entry.size;
                        memcpy(tnode->inode->symlink, file->linked_file_name,
                               tnode->inode->size);
                    }
                } else {
                    if (filesize > 0) {
                        item->entry.data = (void *)(kmalloc(filesize));
                        if (!(item->entry.data)) {
                            kloge("INIT RAMFS: Failed to allocate memory for item->entry.data!\n");
                            halt();
                        }
                        memcpy(item->entry.data, (void *)(ptr + 512), filesize);
                    } else {
                        item->entry.data = NULL;
                    }
                    tnode = vfs_path_to_node(dir_name, CREATE, VFS_FILE);
                    tnode->inode->size = item->entry.size;
                }

                /* Set file type and mode */
                tnode->inode->permissions = oct2bin((unsigned char *)&file->mode,
                                                    7 & (S_IRWXU & S_IRWXG & S_IRWXO));
                tnode->stat.mode |= tnode->inode->permissions;

                tnode->stat.access_time.TV_SEC = file_time;
                tnode->stat.modify_time.TV_SEC = file_time;
                tnode->stat.status_change_time.TV_SEC = file_time;

                tnode->stat.access_time.TV_NSEC = 0;
                tnode->stat.modify_time.TV_NSEC = 0;
                tnode->stat.status_change_time.TV_NSEC = 0;

                tnode->stat.nlink = 1;

                /* Set the file size visible in userspace */
                if (ustart_type_to_vfs_type(file->type) == VFS_FILE) {
                    tnode->stat.size = tnode->inode->size;
                } else {
                    tnode->stat.size = 0;
                }

                vector_append(&ramfs.files, (void *)item);

                /* Need to set up the right parent node */
                strcpy(item->path, dir_name);
                if (name_index) {
                    dir_name[name_index] = '\0';
                }
                VFS_TNODE *parent_tnode = vfs_path_to_node(dir_name, NO_CREATE, 0);
                if (parent_tnode) {
                    item->parent = parent_tnode->inode;
                    tnode->parent = parent_tnode->inode;
                } else {
                    kloge("INIT RAMFS: %s cannot find parent node\n", file->name);
                }
                break;
            default:
                return;
        }
        ptr += (DIV_ROUNDUP(filesize, 512) + 1) * 512;
    }
    klogs("INIT RAMFS: finished...\n");
}

/**
 * @brief Main open function for ramfs
 *
 * @param this inode to manipulate
 * @param path **FULL PATH** to inode
 * @return VFS_TNODE* tnode from inode
 */
VFS_TNODE *ramfs_open(VFS_INODE *this, const char *path) {
    char p[VFS_MAX_PATH_LEN];
    strcpy(p, path);

    /* TODO: Remove '.' in full path name here */
    size_t plen = strlen(path);
    size_t i = 0;
    for (; i + 4 < plen; i++) {
        if (path[i] == '/' && path[i + 1] == '.' && path[i + 2] == '.' &&
            path[i + 3] == '/') {
            int found_parent = 0;
            for (int64_t k = i - 1; k >= 0; k--) {
                if (path[k] == '/') {
                    strcpy(&p[k], &p[i + 3]);
                    found_parent = 1;
                    i = 0;
                    break;
                }
            }

            if (!found_parent) {
                kloge("RAMFS OPEN: \"%s\" is an invalid path\n", path);
                return NULL;
            }
        }
    }

    plen = strlen(p);
    for (int i = plen - 1; i >= 0; i--) {
        if (p[i] == '/') {
            break;
        }
    }

    RAMFS_IDENT *id = (RAMFS_IDENT *)(this->ident);
    if (!id) {
        id = (RAMFS_IDENT *)(kmalloc(sizeof(RAMFS_IDENT)));
        if (!id) {
            kloge("RAMFS OPEN: Failed to allocate memory for new ident!\n");
            halt();
        }

        memset(id, 0, sizeof(RAMFS_IDENT));
    }

    /* TODO: Speed up this part and check where to free id->data */
    int is_link = 0;
    char linkpath[VFS_MAX_PATH_LEN] = {0};
    for (size_t i = 0; i < vector_len(&ramfs.files); i++ ) {
        RAMFS_IDENT_ITEM *item = vector_at(&ramfs.files, i);
        if (!strcmp(item->path, p) && strlen(p) > 0) {
            if (item->type == VFS_SYMLINK) {
                is_link = 1;
                strcpy(linkpath, item->path);
                if (((char *)item->entry.data)[0] == '/') {
                    strcpy(linkpath, (char *)(item->entry.data));
                } else {
                    strcpy(linkpath, p);
                    for (int64_t k = strlen(linkpath) - 1; k >= 0; k--) {
                        if (linkpath[k] == '/') {
                            linkpath[k + 1] = '\0';
                            break;
                        }
                    }
                    strcat(linkpath, (char *)(item->entry.data));
                }
                break;
            }

            /* TODO: Refactor these couple of if's */

            if (item->entry.size == 0) {
                if (id->data) {
                    kfree(id->data);
                    id->alloc_size = 0;
                    break;
                }
            }

            if (id->data) {
                kfree(id->data);
            }

            id->data = (void *)(kmalloc(item->entry.size));
            if (!id->data) {
                kloge("RAMFS OPEN: Failed to allocate space for id->data!\n");
                halt();
            }
            id->alloc_size = item->entry.size;

            memcpy(id->data, item->entry.data, item->entry.size);
            break;
        }
    }

    if (is_link) {
        for (size_t i = 0; i < vector_len(&ramfs.files); i++) {
            RAMFS_IDENT_ITEM *item = vector_at(&ramfs.files, i);
            if (!strcmp(item->path, linkpath)) {
                if (item->type != VFS_FILE) {
                    continue;
                }

                if (item->entry.size == 0) {
                    if (id->data) {
                        kfree(id->data);
                        id->data = NULL;
                        id->alloc_size = 0;
                        break;
                    }
                }

                id->data = (void *)(krealloc(id->data, item->entry.size));
                id->alloc_size = item->entry.size;
                memcpy(id->data, item->entry.data, item->entry.size);
                break;
            }
        }
    }

    VFS_TNODE *tnode = vfs_path_to_node(path, NO_CREATE, 0);
    if (tnode && is_link) {
        tnode->inode->size = id->alloc_size;
    }

    return tnode;
}

/**
 * @brief Main ramfs read function
 *
 * @param this inode to read
 * @param offset Offset to start reading from
 * @param len Number of bytes to read
 * @param buff Buffer to copy into
 * @return int64_t Number of bytes read
 */
int64_t ramfs_read(VFS_INODE *this, size_t offset, size_t len, void *buff) {
    RAMFS_IDENT *id = (RAMFS_IDENT *)(this->ident);

    size_t retlen = len;
    if (offset + retlen > id->alloc_size) {
        retlen = id->alloc_size - offset;
    }
    if (offset > id->alloc_size) {
        retlen = 0;
    }
    if (retlen) {
        memcpy(buff, ((uint8_t *)(id->data)) + offset, len);
    } else {
        kloge("RAMFS READ: read %d bytes from %x with offset %d but failed with"
              "copy (%d <= %d)\n", len, id->data, offset, offset, id->alloc_size);
    }

    return retlen;
}

/**
 * @brief Main ramfs node removal function
 *
 * @param this tnode to remove
 * @return int64_t 0 if success, -1 if failure
 */
int64_t ramfs_rmnode(VFS_TNODE *this) {
    /* TODO */
    (void)this;
    return -1;
}

/**
 * @brief Main ramfs writing function
 *
 * @param this inode to manipulate
 * @param offset Offset to start writing from
 * @param len Number of bytes to write
 * @param buff Buffer to write
 * @return int64_t Number of bytes written
 */
int64_t ramfs_write(VFS_INODE *this, size_t offset, size_t len, const void *buff) {
    RAMFS_IDENT *id = (RAMFS_IDENT *)(this->ident);

    /* Update the size if needed */
    if (offset + len > this->size) {
        this->size = offset + len;
    }

    /* Update buffer size if needed */
    if (this->size > id->alloc_size) {
        id->alloc_size = this->size;
        id->data = krealloc(id->data, id->alloc_size);
    }

    memcpy(((uint8_t *)id->data) + offset, buff, len);
    return len;
}

/**
 * @brief Main sync function for ramfs
 *
 * @param this inode to sync
 * @return int64_t 0 if success, -1 if failure
 */
int64_t ramfs_sync(VFS_INODE *this) {
    RAMFS_IDENT *id = (RAMFS_IDENT *)(this->ident);

    if (this->size > id->alloc_size) {
        id->alloc_size = this->size;
        id->data = krealloc(id->data, id->alloc_size);
    }

    return 0;
}

/**
 * @brief Main link setting function for ramfs
 *
 * @param this tnode to set a link to
 * @param inode inode which is being set as link
 * @return int64_t 0 if success, -1 if failure
 */
int64_t ramfs_setlink(VFS_TNODE *this, VFS_INODE *inode) {
    /* TODO */
    (void)this;
    (void)inode;
    return -1;
}


/**
 * @brief Main inode refresh function for ramfs
 *
 * @param this inode to refresh
 * @return int64_t 0 if success, -1 if failure
 */
int64_t ramfs_refresh(VFS_INODE *this) {
    (void)this;
    return 0;
}

/**
 * @brief Main directory entry getter for ramfs
 *
 * @param this inode to get directory entry of
 * @param pos Position
 * @param dirent Buffer to copy directory entry into
 * @return int64_t 0 if success, -1 if failure
 */
int64_t ramfs_getdent(VFS_INODE *this, size_t pos, VFS_DIR_ENTRY *dirent) {
    size_t num = 0;
    for (size_t i = 0; i < vector_len(&ramfs.files); i++) {
        RAMFS_IDENT_ITEM *item = vector_at(&ramfs.files, i);
        if ((uint64_t)item->parent != (uint64_t)this) {
            continue;
        } else if (num == pos) {
            strcpy(dirent->name, item->name);
            memcpy(&dirent->time, &item->time, sizeof(STD_TIME));
            dirent->type = VFS_FILE;
            dirent->size = item->entry.size;
            return 0;
        }
        num++;
    }

    return -1;
}

/**
 * @brief Main node making function for ramfs
 *
 * @param this tnode to make
 * @return int64_t 0 if success, -1 if failure
 */
int64_t ramfs_mknode(VFS_TNODE *this) {
    this->inode->ident = create_ident();
    return 0;
}

/**
 * @brief Main mounting function for ramfs
 *
 * @param at Location to mount at
 * @return VFS_INODE* New inode of the mounted filesystem
 */
VFS_INODE *ramfs_mount(VFS_INODE *at) {
    (void) at;
    klogi("RAMFS MOUNT: Mounting ramfs to %x\n", at);
    VFS_INODE *ret = vfs_alloc_inode(VFS_MOUNT_POINT, 0777, 0, &ramfs, NULL);
    ret->ident = create_ident();
    return ret;
}