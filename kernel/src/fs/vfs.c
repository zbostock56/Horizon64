/**
 * @file vfs.c
 * @author Zack Bostock
 * @brief Code related to the VFS layer
 * @verbatim
 * A virtual file system (VFS) or virtual filesystem switch is an abstract layer
 * on top of a more concrete file system. The purpose of a VFS is to allow client
 * applications to access different types of concrete file systems in a uniform way.
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <common/lock.h>
#include <common/kmalloc.h>
#include <common/kprint.h>
#include <common/vector.h>
#include <common/string.h>
#include <common/hash.h>

#include <fs/vfs.h>

#include <proc/ctxsw.h>
#include <proc/syscall.h>

#include <sys/cmos.h>
#include <sys/acpi/hpet.h>
#include <sys/smp.h>

/**
 * @brief VFS-wide locks
 */
LOCK vfs_lock = {0};
LOCK dev_lock = {0};
LOCK ino_lock = {0};

/**
 * @brief Different filesystem structures
 */
extern VFS_FS ttyfs;
extern VFS_FS pipefs;
extern VFS_FS ramfs;
extern VFS_FS fat32;

/**
 * @brief New ID numbers
 */
static DEVICE next_new_dev_id = 1;
static INO next_new_ino_id = 1;
static uint8_t vfs_initialized = FALSE;
static size_t vfs_next_handle = VFS_MIN_HANDLE;

/**
 * @brief Root node
 */
VFS_TNODE vfs_root = {0};

/**
 * @brief List of installed filesystems
 */
vector_new_static(VFS_FS *, vfs_fs_list);

/**
 * @brief Helper for getting a new device ID
 *
 * @return DEVICE New device ID
 */
DEVICE vfs_new_dev_id() {
    DEVICE d;

    LOCK_LOCK(&dev_lock);
    d = next_new_dev_id++;
    UNLOCK_LOCK(&dev_lock);

    return d;
}

/**
 * @brief Helper for getting a new inode ID
 *
 * @return INO New inode number
 */
INO vfs_new_ino_id() {
    INO i;

    LOCK_LOCK(&ino_lock);
    i = next_new_ino_id++;
    UNLOCK_LOCK(&ino_lock);

    return i;
}

/**
 * @brief Helper to add a new filesystem
 *
 * @param fs Filesystem to add
 */
void vfs_register_fs(VFS_FS *fs) {
    vector_append(&vfs_fs_list, fs);
}

/**
 * @brief Helper to find if filesystem is mounted
 *
 * @param name Name of filesystem to search for
 * @return VFS_FS* Pointer to FS if found, NULL otherwise
 */
VFS_FS *vfs_search_fs(char *name) {
    for (size_t i = 0; i < vfs_fs_list.length; i++) {
        if (!strncmp(name, vfs_fs_list.data[i]->name, sizeof(((VFS_FS) {0}).name))) {
            return vfs_fs_list.data[i];
        }
    }
    kloge("File system %s was not found!\n");
    return NULL;
}

/**
 * @brief Helper function to create a new tnode
 *
 * @param name Name for the tnode
 * @param inode Actual inode
 * @param parent Parent inode
 * @return VFS_TNODE* Newly allocated tnode
 */
VFS_TNODE *vfs_alloc_tnode(const char *name, VFS_INODE *inode, VFS_INODE *parent) {
    VFS_TNODE *tnode = (VFS_TNODE *) (kmalloc(sizeof(VFS_TNODE)));
    if (!tnode) {
        kloge("Failed to allocate a new tnode!\n");
        return NULL;
    }

    memset(tnode, 0, sizeof(VFS_TNODE));
    memcpy(tnode->name, name, sizeof(tnode->name));

    tnode->inode = inode;
    tnode->parent = parent;
    tnode->stat.dev = vfs_new_dev_id();
    tnode->stat.ino = vfs_new_ino_id();

    return tnode;
}

/**
 * @brief Helper to create a new inode
 *
 * @param type Node type
 * @param perms Permissions for the inode
 * @param uid ID for the inode
 * @param fs Filesystem which this inode is related to
 * @param mount_point Mount point of the inode
 * @return VFS_INODE* Newly allocated inode
 */
VFS_INODE *vfs_alloc_inode(VFS_NODE_TYPE type, uint32_t perms, uint32_t uid,
                           VFS_FS *fs, VFS_TNODE *mount_point) {
    VFS_INODE *inode = (VFS_INODE *) (kmalloc(sizeof(VFS_INODE)));
    if (!inode) {
        kloge("Failed to allocate a new inode!\n");
        return NULL;
    }

    memset(inode, 0, sizeof(VFS_INODE));

    inode->type = type;
    inode->permissions = perms;
    inode->uid = uid;
    inode->fs = fs;
    inode->ident = NULL;
    inode->mount_point = mount_point;
    inode->references = 0;
    inode->size = 0;

    return inode;
}

/**
 * @brief Helper to free a tnode
 *
 * @param tnode tnode to free
 * @return STATUS SYS_OK if success, SYS_ERR if failure
 */
STATUS vfs_free_nodes(VFS_TNODE *tnode) {
    if (!tnode) {
        return SYS_ERR;
    }
    VFS_INODE *inode = tnode->inode;

    /* If nothing else is referencing the inode, then free it */
    if (inode->references <= 0) {
        kfree(inode);
    }
    kfree(tnode);

    return SYS_OK;
}

/**
 * @brief Helper to return node descriptor for a file handle
 *
 * @param h Handle to search for
 * @return VFS_NODE_DESC* Node descriptor which describes handle, can be NULL
 */
VFS_NODE_DESC *vfs_handle_to_fd(VFS_HANDLE h) {
    PROCESS *pcurr = sched_get_curr_proc();
    if (pcurr) {
        VFS_NODE_DESC *nd = (VFS_NODE_DESC *)(hash_search(&(pcurr->open_files), h));
        if (nd) {
            return nd;
        }
        kloge("VFS: Unable to locate handle %d in file list of process %d."
              "Might need to increase hash table's size.\n", h, pcurr->id);
    }
    return NULL;
}

/**
 * @brief Gets a VFS_TNODE from a path
 *
 * @param path_name Path name to search with
 * @param mode Mode of the node
 * @param type Type of the node
 * @return VFS_TNODE* NULL if not found, VFS_TNODE found otherwise
 */
VFS_TNODE *vfs_path_to_node(const char *path_name, uint8_t mode, VFS_NODE_TYPE type) {
    char temp_buff[VFS_MAX_PATH_LEN];
    char path[VFS_MAX_PATH_LEN];

    VFS_TNODE *curr = &vfs_root;

    /* NOTE: only works with absolute paths */
    /* TODO: Check for NULL pointer deferences here - potentially refactor */
    if (path_name[0] != '/') {
        if (sys_get_full_path(VFS_FW_CWD, path_name, temp_buff) == SYSCALL_FAIL) {
            kloge("'%s' is not a vaild path!\n", path_name);
            return NULL;
        }
        strcpy(path, &(temp_buff[1]));
    } else {
        path_name++;
        strcpy(path, path_name);    /* Skip the leading slash here */
    }

    /* TODO: Remove the '.' in the fll path name here */

    size_t pathlen = strlen(path);
    size_t i = 0;

    for (; i + 4 < pathlen; i++) {
        if (path[i] == '/' && path[i + 1] == '.' && path[i + 2] == '.' && path[i + 3] == '/') {
            int found_parent = 0;
            for (int64_t k = i - 1; k >= 0; k--) {
                if (path[k] == '/') {
                    strcpy(&path[k], &path[i + 3]);
                    found_parent = 1;
                    i = 0;
                    break;
                }
            }

            if (!found_parent) {
                kloge("'%s' is an invalid path\n", path_name);
                return NULL;
            }
        }
    }

    if (strlen(path_name) != strlen(path)) {
        klogw("VFS: \"%s\" -> \"%s\"\n", path_name, path);
    }

    pathlen = strlen(path);
    size_t curr_index = 0;
    int found_node = 1;
    for (; curr_index < pathlen;) {
        /* Extract next token from path */
        for (i = 0; curr_index + i < pathlen; i++) {
            if (path[curr_index + i] == '/') {
                break;
            }
            temp_buff[i] = path[curr_index + i];
        }

        temp_buff[i] = '\0';
        curr_index += i + 1;

        if (!strcmp(temp_buff, ".")) {
            continue;
        }

        /* Search for token in children of current node */
        found_node = 0;
        if (!IS_TRAVERSABLE(curr->inode)) {
            break;
        }

        for (i = 0; i < curr->inode->child.length; i++) {
            VFS_TNODE *child = vector_at(&(curr->inode->child), i);
            if (!strncmp(child->name, temp_buff, sizeof(child->name))) {
                found_node = 1;
                curr = child;
                break;
            }
        }

        /* TODO: Issue with /usr/local/include -> /usr/include */
        if (!found_node) {
            break;
        }
    }

    /* Should we create the node */
    if (!found_node) {
        /* Only directories can contain files */
        if (!IS_TRAVERSABLE(curr->inode)) {
            kloge("'%s' does not reside in a directory!\n", path);
            return NULL;
        }

        /*
            Create the node if CREATE was specified and the node to be
            created is the last one in the path
        */

        if ((mode & CREATE) && curr_index > pathlen && IS_TRAVERSABLE(curr->inode)) {
            /* Permissions: 777 */
            VFS_INODE *new_inode = vfs_alloc_inode(type, 0, 0, curr->inode->fs,
                                                   curr->inode->mount_point);

            uint64_t now_seconds = NANOS_TO_SECONDS(hpet_get_nanos());
            uint64_t boot_seconds = cmos_get_boot_time_seconds();

            /* Add the creation time into the inode */
            seconds_to_std_time(now_seconds + boot_seconds, &(new_inode->time));

            VFS_TNODE *new_tnode = vfs_alloc_tnode(temp_buff, new_inode, curr->inode);
            vector_append(&(curr->inode->child), new_tnode);

            if (curr->inode->fs) {
                curr->inode->fs->mknode(new_tnode);
                new_tnode->inode->fs = curr->inode->fs;
            }
            if (!strncmp(path, "usr/local", 9) || !strncmp(path, "/usr/bin", 8)) {
                klogi("VFS: Create \"%s\" node\n", path);
            }

            /* Set the file mode and type */
            switch (type) {
                case VFS_DIRECTORY:
                    new_tnode->stat.mode |= S_IFDIR;
                    new_tnode->stat.nlink = 1;
                    break;
                case VFS_FILE:
                    new_tnode->stat.mode |= S_IFREG;
                    new_tnode->stat.nlink = 1;
                    break;
                case VFS_SYMLINK:
                    new_tnode->stat.mode |= S_IFLNK;
                    new_tnode->stat.nlink = 1;
                    break;
                case VFS_MOUNT_POINT:
                    new_tnode->stat.mode |= S_IFLNK;
                    new_tnode->stat.nlink = 1;
                    break;
                default:
                    break;
            }
            return new_tnode;
        } else {
            kloge("VFS: \"%s\" doesn't exist\n", path);
            cpu_set_errno(ENOENT);
            return NULL;
        }
    } else if (mode & ERR_ON_EXIST) {
        /* the node should have node existed */
        kloge("VFS: \"%s\" already existed\n", path);
        return NULL;
    }

    /* Found the node without needing to make a new one, return it */
    return curr;
}

/**
 * @brief Creates a new TNODE
 *
 * @param path Where the TNODE will be located at
 * @param type Node type
 * @return STATUS SYS_ERR if failure, SYS_OK if success
 */
STATUS vfs_create(char *path, VFS_NODE_TYPE type) {
    STATUS s;
    LOCK_LOCK(&vfs_lock);

    VFS_TNODE *tnode = vfs_path_to_node(path, CREATE, type);
    if (!tnode) {
        s = SYS_ERR;
    } else {
        uint64_t now = NANOS_TO_SECONDS(hpet_get_nanos()) + cmos_get_boot_time_seconds();

        tnode->stat.access_time.TV_SEC = now;
        tnode->stat.modify_time.TV_SEC = now;
        tnode->stat.status_change_time.TV_SEC = now;

        tnode->stat.access_time.TV_NSEC = 0;
        tnode->stat.modify_time.TV_NSEC = 0;
        tnode->stat.status_change_time.TV_NSEC = 0;
        s = SYS_OK;
    }
    UNLOCK_LOCK(&vfs_lock);
    return s;
}

/**
 * @brief Changes the permissions of a handle
 *
 * @param h Handle to change
 * @param perms New permissions
 * @return STATUS SYS_ERR if fail, SYS_OK if success
 */
STATUS vfs_chmod(VFS_HANDLE h, int32_t perms) {
    VFS_NODE_DESC *fd = vfs_handle_to_fd(h);
    if (!fd) {
        /* Bad file descriptor */
        cpu_set_errno(EBADF);
        return SYS_ERR;
    }

    if (fd->mode == VFS_READ) {
        kloge("VFS: Opened as read-only\n");
        return SYS_ERR;
    }

    /* Set new permissions */
    fd->inode->permissions = perms & (S_IRWXU | S_IRWXG | S_IRWXO);
    fd->tnode->stat.mode |= fd->inode->permissions;
    fd->inode->fs->sync(fd->inode);
    return SYS_OK;
}

/**
 * @brief Controls underlying devices
 *
 * @param h Handle to manipulate
 * @param request Operation to request to complete
 * @param arg Arguments to go along with the operation
 * @return int64_t Value from underlying device which was manipulated
 */
int64_t vfs_ioctl(VFS_HANDLE h, int64_t request, int64_t arg) {
    VFS_NODE_DESC *fd = vfs_handle_to_fd(h);
    if (!fd) {
        /* Bad file descriptor */
        cpu_set_errno(EBADF);
        return -1;
    }

    if (fd->inode->fs->ioctl) {
        return fd->inode->fs->ioctl(fd->inode, request, arg);
    }

    return -1;
}

/**
 * @brief Mounts a device
 *
 * @param device Device to mount
 * @param path Path to mount at
 * @param fs_name Name of filesystem to mount on
 * @return STATUS SYS_ERR if fail, SYS_OK if success
 */
STATUS vfs_mount(char *device, char *path, char *fs_name) {
    LOCK_LOCK(&vfs_lock);

    /* Get the filesystem information */
    VFS_FS *fs = vfs_search_fs(fs_name);
    if (!fs) {
        /* Invalid filesystem */
        cpu_set_errno(EINVAL);
        UNLOCK_LOCK(&vfs_lock);
        return SYS_ERR;
    }

    /* Get block device if needed */
    VFS_TNODE *dev = NULL;
    if (!fs->is_temp) {
        dev = vfs_path_to_node(device, NO_CREATE, 0);
        if (!dev) {
            /* Device could not be found */
            cpu_set_errno(EACCES);
            UNLOCK_LOCK(&vfs_lock);
            return SYS_ERR;
        }
        if (dev->inode->type != VFS_BLOCK_DEV) {
            /* Not a block device */
            cpu_set_errno(ENOTBLK);
            UNLOCK_LOCK(&vfs_lock);
            return SYS_ERR;
        }
    }

    /* Get the node where it is to be mounted (should be an empty folder) */
    VFS_TNODE *at = vfs_path_to_node(path, NO_CREATE, 0);
    if (!at) {
        /* Not existant mounting location */
        cpu_set_errno(EINVAL);
        UNLOCK_LOCK(&vfs_lock);
        return SYS_ERR;
    }
    if (at->inode->type != VFS_DIRECTORY || at->inode->child.length != 0) {
        /* Not a directory, or one that already has things in it */
        cpu_set_errno(ENOTDIR);
        kloge("VFS: \"%s\" is not a directory, or is not empty!\n", path);
        UNLOCK_LOCK(&vfs_lock);
        return SYS_ERR;
    }

    kfree(at->inode);


    /* Now, mount the filesystem */

    at->inode = fs->mount(dev ? dev->inode : NULL);
    at->inode->mount_point = at;

    klogi("VFS: mounted %s at %s as %s\n",
           device ? device : "<no-device>", path, fs_name);
    UNLOCK_LOCK(&vfs_lock);
    return SYS_OK;
}

/**
 * @brief Set the length of a file
 *
 * @param h Handle to find size of
 * @return int64_t -1 if failure, size of the file otherwise
 */
int64_t vfs_tell(VFS_HANDLE h) {
    VFS_NODE_DESC* fd = vfs_handle_to_fd(h);

    if (!fd) {
        kloge("VFS: Cannot find file descriptor for file %d\n", h);
        return -1;
    } else {
        return fd->inode->size;
    }
}

/**
 * @brief Reads from a file handle
 *
 * @param h Handle to read from
 * @param len Number of bytes to read
 * @param buff Buffer to copy contents into
 * @return int64_t -1 if failure, number of bytes read upon success
 */
int64_t vfs_read(VFS_HANDLE h, size_t len, void *buff) {
    klogd("VFS Read: Reading %d bytes from %d handle into %x address\n", len,
            h, buff);
    VFS_NODE_DESC *fd = vfs_handle_to_fd(h);
    if (!fd) {
        /* Bad file descriptor */
        cpu_set_errno(EBADF);
        return -1;
    }

    if (fd->inode->type == VFS_DIRECTORY) {
        /* Trying to read a directory inode */
        cpu_set_errno(EISDIR);
        return -1;
    }

    LOCK_LOCK(&vfs_lock);

    VFS_INODE *inode = fd->inode;

    /* Truncate if asking for more data than available */
    if (fd->seek_position + len > inode->size &&
        !strcmp(fd->inode->fs->name, "ttyfs") &&
        !strcmp(fd->inode->fs->name, "pipefs")) {
        len = inode->size - fd->seek_position;
        if (len == 0) {
            UNLOCK_LOCK(&vfs_lock);
            return (int64_t) len;
        }
    }

    int64_t ret = fd->inode->fs->read(fd->inode, fd->seek_position, len, buff);
    if (ret < 0) {
        /* Error occured */
        len = 0;
    } else {
        len = ret;
    }

    fd->seek_position += len;

    UNLOCK_LOCK(&vfs_lock);
    return (int64_t) len;
}

/**
 * @brief Unlink file, reduces the link count. If the reference count is zero
 * then the file is deleted
 *
 * @param path Path of file to unlink
 * @return int64_t 0 if success, -1 if failure
 */
int64_t vfs_unlink(char *path) {
    klogi("VFS: unlinking %s\n", path);

    LOCK_LOCK(&vfs_lock);

    /* Find the node and set the nlink parameter */
    VFS_TNODE *tnode = vfs_path_to_node(path, NO_CREATE, 0);
    if (!tnode) {
        kloge("VFS: Cannot find tnode for %s\n", path);
        /* Path doesn't relate to any tnode */
        cpu_set_errno(EINVAL);
        return -1;
    } else {
        if (tnode->inode->type == VFS_DIRECTORY) {
            kloge("VFS: Unlink: \"%s\" refers to a directory\n", path);
            cpu_set_errno(EISDIR);
            return -1;
        } else if (tnode->stat.nlink > 1) {
            kloge("VFS: Unlink: \"%s\" has links which should be removed first\n", path);
            return -1;
        } else if (tnode->stat.nlink == 0) {
            kloge("VFS: Unlink: \"%s\" should only have one link, but has zero\n", path);
            return -1;
        }

        tnode->stat.nlink = 0;
    }

    /* Remove the file if needed */
    if (tnode->inode->references == 0) {
        if (tnode->inode->fs->rmnode) {
            tnode->inode->fs->rmnode(tnode);
        }
    }

    UNLOCK_LOCK(&vfs_lock);
    return 0;
}

/**
 * @brief Writes specified number of bytes to a file
 *
 * @param h Handle to write to
 * @param buf Buffer with data to write
 * @param count Number of bytes to write
 * @return int64_t 0 if success, -1 if failure
 */
int64_t vfs_write(VFS_HANDLE h, const void *buf, size_t count) {
    if (!buf) {
        /* Buf is outside the accessable address space */
        cpu_set_errno(EFAULT);
        return -1;
    }

    VFS_NODE_DESC *fd = vfs_handle_to_fd(h);
    if (!fd) {
        /* Bad file descriptor */
        cpu_set_errno(EBADF);
        return -1;
    }

    if (fd->mode == VFS_READ) {
        kloge("VFS: Write: Trying to write to a read-only node with %d handle\n", h);
        return -1;
    }

    LOCK_LOCK(&vfs_lock);
    VFS_INODE *inode = fd->inode;

    /* Expand file if writing more data than its size */
    if (fd->seek_position + count > inode->size) {
        inode->size = fd->seek_position + count;
        if (inode->fs->sync) {
            inode->fs->sync(inode);
        }
    }

    if (inode->fs->write(inode, fd->seek_position, count, buf) == -1) {
        count = 0;
    } else {
        /* Move seek position to tail of writing area */
        fd->seek_position += count;
    }

    /* Set file size to stat data structure */
    fd->tnode->stat.size = fd->inode->size;

    UNLOCK_LOCK(&vfs_lock);
    return count;
}

/**
 * @brief Reposition read/write file offset
 *
 * @param h Handle to reposition
 * @param offset Offset in file
 * @param whence Seek mode
 * @return int64_t -1 if failure, otherwise returns resulting offset location in file
 * measured from the beginning of the file
 */
int64_t vfs_seek(VFS_HANDLE h, size_t offset, int whence) {
    VFS_NODE_DESC *fd = vfs_handle_to_fd(h);
    if (!fd) {
        /* Bad file descriptor */
        cpu_set_errno(EBADF);
        return -1;
    }

    LOCK_LOCK(&vfs_lock);

    int64_t pos = -1;
    switch(whence) {
        case SEEK_SET:
            pos = offset;
            break;
        case SEEK_CUR:
            pos = fd->seek_position + offset;
            break;
        case SEEK_END:
            pos = fd->inode->size - offset;
            break;
        default:
            /* Whence input is not valid */
            UNLOCK_LOCK(&vfs_lock);
            cpu_set_errno(EINVAL);
            return -1;
    }

    /* For writing mode, it can enlarge file size */
    if ((fd->mode == VFS_WRITE || fd->mode == VFS_READ_WRITE) &&
         pos > (int64_t)fd->inode->size) {
        fd->inode->size = pos;
        if (fd->inode->fs->sync) {
            fd->inode->fs->sync(fd->inode);
        }
    }

    if (pos > (int64_t)fd->inode->size || pos < 0) {
        kloge("VFS: seek: position out of bounds %d: %d in len %d with offset %d\n",
               pos, whence, fd->inode->size, fd->seek_position);
        UNLOCK_LOCK(&vfs_lock);
        /* Whence is not valid. Or: the resulting file offset would be negative */
        /* or beyond the end of a seekable device */
        cpu_set_errno(EINVAL);
        return -1;
    }

    int64_t ret = -1;
    /* pos could never be <= to zero due to the check above here */
    if (pos > 0 && pos <= (int64_t)fd->inode->size) {
        fd->seek_position = pos;
        ret = pos;
    }

    UNLOCK_LOCK(&vfs_lock);
    return ret;
}

/**
 * @brief Gets the parent directory of a path
 *
 * @param path Path to find parent of
 * @param parent Buffer to copy into
 * @param curr_dir Current directory
 * @return int64_t -1 if fail, 0 if success
 */
int64_t vfs_get_parent_dir(const char *path, char *parent, char *curr_dir) {
    if (!path || !parent) {
        return -1;
    }

    strcpy(parent, path);

    int64_t i = strlen(parent) - 1;
    while (i > 0) {
        if (parent[i] == '/') {
            parent[i] = '\0';
            i--;
        }
        if (parent[i] != '/') {
            break;
        }
    }

    /* Does not have a parent directory */
    if (i <= 0) {
        parent[0] = '\0';
        return -1;
    }

    /* Does have a parent directory */
    while (i >= 0) {
        if (parent[i] == '/') {
            parent[i] = '\0';
            break;
        }
        i--;
    }

    if (curr_dir && i >= 0) {
        strcpy(curr_dir, &(parent[i + 1]));
    }
    if (strlen(parent) == 0) {
        strcpy(parent, "/");
    }

    return 0;
}

/**
 * @brief Open and possibly create a file
 *
 * @param path Path to open
 * @param mode Mode to open file with
 * @return VFS_HANDLE Invalid if no openable, otherwise handle related to the file
 */
VFS_HANDLE vfs_open(char *path, VFS_OPEN_MODE mode) {
    LOCK_LOCK(&vfs_lock);

    /* Find the node */
    VFS_TNODE *req = vfs_path_to_node(path, NO_CREATE, 0);
    if (!req) {
        kloge("VFS: Open: Cannot find inode for %s\n", path);
        VFS_TNODE *tnode = NULL;
        char curr_path[VFS_MAX_PATH_LEN] = {0};
        char parent[VFS_MAX_PATH_LEN] = {0};
        strcpy(curr_path, path);
        while (TRUE) {
            if (vfs_get_parent_dir(curr_path, parent, NULL) == -1) {
                kloge("VFS: Open: Failed to get parent directory!\n");
                return VFS_INVALID_HANDLE;
            }
            if (!strcmp(curr_path, parent)) {
                break;
            }
            tnode = vfs_path_to_node(parent, NO_CREATE, 0);
            if (tnode) {
                break;
            }
            strcpy(curr_path, parent);
        }

        if (tnode && tnode->inode->fs) {
            kloge("VFS: Open: Can not open %s, visit back to %s\n", path, parent);
            req = tnode->inode->fs->open(tnode->inode, path);
        }
        if (!req) {
            UNLOCK_LOCK(&vfs_lock);
            kloge("VFS: Open: Failed to open %s with mode %x", path, mode);
            return VFS_INVALID_HANDLE;
        }
    } else {
        /* Move forward to open the file */
        if (req->inode->fs) {
            klogi("VFS: Open: inode for %s already exists\n", path);
            req = req->inode->fs->open(req->inode, path);
        }
    }

    req->inode->references++;

    /* Create node descriptor */
    VFS_NODE_DESC *nd = (VFS_NODE_DESC *)(kmalloc(sizeof(VFS_NODE_DESC)));
    memset(nd, 0, sizeof(VFS_NODE_DESC));

    strcpy(nd->path, path);
    nd->tnode = req;
    nd->inode = req->inode;
    nd->seek_position = 0;
    nd->mode = mode;

    /*
        TODO: If this is a symbolic link, need to set the real file size. Should
              develop an alrgorithm to search through symbolic links
    */
    nd->tnode->stat.size = req->inode->size;

    /* Return the handle */
    VFS_HANDLE h = vfs_next_handle++;

    /* Add to current process */
    PROCESS *pcurr = sched_get_curr_proc();
    if (pcurr) {
        hash_insert(&(pcurr->open_files), h, nd);
    } else {
        kloge("VFS: Open: Cannot insert \"%s\" because of invalid process\n", path);
    }

    UNLOCK_LOCK(&vfs_lock);

    klogi("VFS Open: Opened %s with mode %x and return handle %d, "
          "node desc: %x, inode: %x\n", path, mode, h, nd, nd->inode);
    return h;
}

/**
 * @brief Closes a handle
 *
 * @param h Handle to close
 * @return int64_t 0 if success, -1 if failure
 */
int64_t vfs_close(VFS_HANDLE h) {
    klogd("VFS Close: Closing handle %d\n", h);
    LOCK_LOCK(&vfs_lock);

    VFS_NODE_DESC *nd = vfs_handle_to_fd(h);
    if (!nd) {
        UNLOCK_LOCK(&vfs_lock);
        cpu_set_errno(EINVAL);
        return -1;
    }

    if (!strcmp(nd->path, "/dev/tty")) {
        klogi("VFS: close /dev/tty with file handle %d\n", h);
    }
    nd->inode->references--;

    /* Remove from current process */
    PROCESS *pcurr = sched_get_curr_proc();
    if (pcurr) {
        hash_delete(&(pcurr->open_files), h);
    } else {
        kloge("VFS: Close: Cannot remove %d because of invalid process!\n", h);
    }

    /* Remove the file if needed */
    if (nd->inode->references == 0 && nd->tnode->stat.nlink == 0) {
        if (nd->inode->fs->rmnode) {
            klogi("VFS: Close: close \"%s\" and remove tnode\n", nd->path);
            nd->inode->fs->rmnode(nd->tnode);
        }
    }

    kfree(nd);
    UNLOCK_LOCK(&vfs_lock);
    return 0;
}

/**
 * @brief Refreshs a handle in the underlying filesystem
 *
 * @param h Handle to refresh
 * @return int64_t -1 if failure, 0 if success
 */
int64_t vfs_refresh(VFS_HANDLE h) {
    VFS_NODE_DESC *nd = vfs_handle_to_fd(h);
    if (!nd) {
        return -1;
    }

    LOCK_LOCK(&vfs_lock);
    nd->inode->fs->refresh(nd->inode);
    for (size_t i = 0;; i++) {
        VFS_DIR_ENTRY de;
        if (nd->inode->fs->getdent(nd->inode, i, &de)) {
            break;
        }

        char path[VFS_MAX_PATH_LEN] = {0};
        strcpy(path, nd->path);
        strcat(path, "/");
        strcat(path, de.name);
        VFS_TNODE *tnode = vfs_path_to_node(path, CREATE, de.type);
        memcpy(&tnode->inode->time, &de.time, sizeof(STD_TIME));
        tnode->inode->size = de.size;
    }

    UNLOCK_LOCK(&vfs_lock);

    return 0;
}

int64_t vfs_getdent(VFS_HANDLE h, VFS_DIR_ENTRY *dirent) {
    VFS_NODE_DESC *nd = vfs_handle_to_fd(h);
    if (!nd) {
        return -1;
    }

    LOCK_LOCK(&vfs_lock);

    /* Can only traverse folders */
    if (!IS_TRAVERSABLE(nd->inode)) {
        kloge("VFS: getdent: Node not traversable\n");
        UNLOCK_LOCK(&vfs_lock);
        return -1;
    }

    /* TODO: Need to make sure that we already load all children here */

    /* We've reached the end */
    if (nd->seek_position >= nd->inode->child.length) {
        UNLOCK_LOCK(&vfs_lock);
        return -1;
    }

    /* Initialize the directory entry */
    VFS_TNODE *entry = vector_at(&(nd->inode->child), nd->seek_position);
    dirent->type = entry->inode->type;
    memcpy(dirent->name, entry->name, sizeof(entry->name));
    memcpy(&dirent->time, &entry->inode->time, sizeof(STD_TIME));

    /* Advance the offset */
    nd->seek_position++;

    UNLOCK_LOCK(&vfs_lock);
    return 0;
}

/**
 * @brief Main initialization of VFS subsystem
 */
void vfs_init() {
    if (vfs_initialized) {
        return;
    }

    klogs("INIT VFS: starting...\n");

    vfs_initialized = TRUE;

    /* Initialize the root directory */
    vfs_root.inode = vfs_alloc_inode(VFS_DIRECTORY, 0777, 0, NULL, NULL);
    vfs_root.stat.dev = vfs_new_dev_id();
    vfs_root.stat.ino = vfs_new_ino_id();
    vfs_root.stat.mode |= S_IFDIR;
    vfs_root.stat.nlink = 1;

    /* Register all file systems */
    vfs_register_fs(&fat32);
    vfs_register_fs(&ramfs);
    vfs_register_fs(&ttyfs);
    vfs_register_fs(&pipefs);

    /* Mount RAMFS without device name */
    vfs_mount(NULL, "/", "ramfs");

    /* Create directory for mounting devices */
    vfs_path_to_node("/disk", CREATE, VFS_DIRECTORY);
    vfs_path_to_node("/dev", CREATE, VFS_DIRECTORY);

    /* Mount TTYFS with device name "/dev/tty" */
    vfs_path_to_node("/dev/tty", CREATE, VFS_DIRECTORY);
    vfs_mount("tty", "/dev/tty", "ttyfs");

    /* Mount PIPEFS with device name "/dev/pipe" */
    vfs_path_to_node("/dev/pipe", CREATE, VFS_DIRECTORY);
    vfs_mount("pipe", "/dev/pipe", "pipefs");


    klogs("INIT VFS: finished...\n");
}