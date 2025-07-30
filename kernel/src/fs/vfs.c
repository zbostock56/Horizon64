/**
 * @file vfs.c
 * @author Zack Bostock
 * @brief Code related to the VFS layer
 * @verbatim
 * A virtual file system (VFS) or virtual filesystem switch is an abstract layer
 * on top of a more concrete file system. The purpose of a VFS is to allow client
 * applications to access different types of concrete file systems in a uniform way.
 *
 * @copyright Copyright (c) 2025
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
 * @brief Configurable paths for debug logging
 */
static const char *log_paths[] = {"usr/local", "usr/bin"};
static const size_t num_log_paths = sizeof(log_paths) / sizeof(log_paths[0]);

/**
 * @brief Normalize a POSIX-style path, resolving '.' and '..' components.
 *
 * @param dst Destination buffer for normalized path (no leading slash)
 * @param src Source path string (no leading slash)
 * @param dst_size Size of destination buffer
 * @return int 0 on success, -1 on error (e.g. trying to escape above root)
 */
static int normalize_path(char *dst, const char *src, size_t dst_size) {
    char temp[VFS_MAX_PATH_LEN];
    char *components[VFS_MAX_PATH_LEN / 2];
    char *component_storage[VFS_MAX_PATH_LEN / 2];
    int depth = 0;

    if (!dst || !src || dst_size == 0) {
        return -1;
    }

    if (strlen(src) >= sizeof(temp)) {
        return -1;
    }

    strcpy(temp, src);

    char *token = strtok(temp, "/");
    while (token != NULL && depth < (VFS_MAX_PATH_LEN / 2)) {
        if (strcmp(token, "..") == 0) {
            if (depth == 0) {
                /* Trying to go above root */
                for (int i = 0; i < depth; i++) {
                    kfree(component_storage[i]);
                }
                return -1;
            }
            kfree(component_storage[--depth]);
        } else if (strcmp(token, ".") != 0 && strlen(token) > 0) {
            component_storage[depth] = kmalloc(strlen(token) + 1);
            if (!component_storage[depth]) {
                /* Clean up -- allocation failure */
                for (int i = 0; i < depth; i++) {
                    kfree(component_storage[i]);
                }
                return -1;
            }
            strcpy(component_storage[depth], token);
            components[depth] = component_storage[depth];
            depth++;
        }
        token = strtok(NULL, "/");
    }

    /* Rebuild path */
    dst[0] = '\0';
    size_t current_len = 0;
    for (int i = 0; i < depth; i++) {
        if (i > 0) {
            if (current_len + 1 >= dst_size) {
                for (int j = 0; j < depth; j++) {
                    kfree(component_storage[j]);
                }
                return -1;
            }
            strncat(dst, "/", dst_size - current_len - 1);
            current_len++;
        }

        size_t comp_len = strlen(components[i]);
        if (current_len + comp_len >= dst_size) {
            for (int j = 0; j < depth; j++) {
                kfree(component_storage[j]);
            }
            return -1;
        }

        strncat(dst, components[i], dst_size - current_len - 1);
        current_len += comp_len;
    }

    /* Clean up allocated memory */
    for (int i = 0; i < depth; i++) {
        kfree(component_storage[i]);
    }

    return 0;
}

/**
 * @brief Extract next path component safely
 *
 * @param path Full path string
 * @param start_index Starting position in path
 * @param path_len Total length of path
 * @param token_buffer Buffer to store extracted token
 * @param buffer_size Size of token buffer
 * @return size_t Length of extracted token, or 0 if end of path
 */
static size_t extract_path_token(const char *path, size_t start_index, size_t path_len,
                                char *token_buffer, size_t buffer_size) {
    if (!path || !token_buffer || start_index >= path_len || buffer_size == 0) {
        if (token_buffer && buffer_size > 0) {
            token_buffer[0] = '\0';
        }
        return 0;
    }

    size_t token_len = 0;
    while ((start_index + token_len) < path_len &&
           path[start_index + token_len] != '/' &&
           token_len < (buffer_size - 1)) {
        token_buffer[token_len] = path[start_index + token_len];
        token_len++;
    }

    token_buffer[token_len] = '\0';
    return token_len;
}

/**
 * @brief Find child node by name
 *
 * @param parent Parent node to search in
 * @param name Name of child to find
 * @return VFS_TNODE* Found child or NULL if not found
 */
static VFS_TNODE *find_child_node(VFS_TNODE *parent, const char *name) {
    if (!parent || !name || !IS_TRAVERSABLE(parent->inode)) {
        return NULL;
    }

    for (size_t i = 0; i < parent->inode->child.length; i++) {
        VFS_TNODE *child = vector_at(&(parent->inode->child), i);
        if (child && strcmp(child->name, name) == 0) {
            return child;
        }
    }

    return NULL;
}

/**
 * @brief Create a new VFS node
 *
 * @param parent Parent directory node
 * @param name Name of new node
 * @param type Type of node to create
 * @return VFS_TNODE* Created node or NULL on failure
 */
static VFS_TNODE *create_vfs_node(VFS_TNODE *parent, const char *name, VFS_NODE_TYPE type) {
    if (!parent || !name || !IS_TRAVERSABLE(parent->inode)) {
        kloge("Cannot create node '%s' in non-directory\n", name ? name : "<null>");
        cpu_set_errno(ENOTDIR);
        return NULL;
    }

    // Allocate new inode with permissions: 777
    VFS_INODE *new_inode = vfs_alloc_inode(type, 0, 0, parent->inode->fs,
                                           parent->inode->mount_point);
    if (!new_inode) {
        kloge("Failed to allocate inode for '%s'\n", name);
        cpu_set_errno(ENOSPC);
        return NULL;
    }

    /* Set timestamp */
    uint64_t now_seconds = NANOS_TO_SECONDS(hpet_get_nanos());
    uint64_t boot_seconds = cmos_get_boot_time_seconds();
    seconds_to_std_time(now_seconds + boot_seconds, &(new_inode->time));

    /* Allocate new tnode */
    VFS_TNODE *new_tnode = vfs_alloc_tnode(name, new_inode, parent->inode);
    if (!new_tnode) {
        kfree(new_inode);
        kloge("Failed to allocate tnode for '%s'\n", name);
        cpu_set_errno(ENOSPC);
        return NULL;
    }

    /* Add to parent's children */
    vector_append(&(parent->inode->child), new_tnode);

    /* Init filesystem-specific data */
    if (parent->inode->fs && parent->inode->fs->mknode) {
        if (parent->inode->fs->mknode(new_tnode) != 0) {
            /* remove parent on filesystem error */
            parent->inode->child.length--;
            vfs_free_nodes(new_tnode);
            kloge("Filesystem failed to create node '%s'\n", name);
            cpu_set_errno(EIO);
            return NULL;
        }
        new_tnode->inode->fs = parent->inode->fs;
    }

    /* Set file mode and type */
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
            kloge("Unknown node type: %d\n", type);
            break;
    }

    return new_tnode;
}

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
    if (fs) {
        vector_append(&vfs_fs_list, fs);
    }
}

/**
 * @brief Helper to find if filesystem is mounted
 *
 * @param name Name of filesystem to search for
 * @return VFS_FS* Pointer to FS if found, NULL otherwise
 */
VFS_FS *vfs_search_fs(char *name) {
    if (!name) {
        return NULL;
    }

    for (size_t i = 0; i < vfs_fs_list.length; i++) {
        VFS_FS *fs = vfs_fs_list.data[i];
        if (fs && strcmp(name, fs->name) == 0) {
            return fs;
        }
    }
    kloge("File system %s was not found!\n", name);
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
    if (!name || !inode) {
        return NULL;
    }

    VFS_TNODE *tnode = (VFS_TNODE *) (kmalloc(sizeof(VFS_TNODE)));
    if (!tnode) {
        kloge("Failed to allocate a new tnode!\n");
        return NULL;
    }

    memset(tnode, 0, sizeof(VFS_TNODE));

    size_t name_len = strlen(name);
    if (name_len >= sizeof(tnode->name)) {
        name_len = sizeof(tnode->name) - 1;
    }
    memcpy(tnode->name, name, name_len);
    tnode->name[name_len] = '\0';

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
    if (inode && inode->references <= 0) {
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
 * @param mode Mode of the node (e.g. CREATE, ERR_ON_EXIST)
 * @param type Type of the node (e.g. VFS_DIRECTORY, VFS_FILE)
 * @return VFS_TNODE* Returns the found or created node, or NULL on failure.
 */
VFS_TNODE *vfs_path_to_node(const char *path_name, uint8_t mode, VFS_NODE_TYPE type) {
    char temp_buffer[VFS_MAX_PATH_LEN];
    char normalized_path[VFS_MAX_PATH_LEN];
    char token_buffer[VFS_MAX_NAME_LEN];
    VFS_TNODE *current_node = &vfs_root;
    VFS_TNODE *result = NULL;

    if (!path_name) {
        kloge("NULL path provided\n");
        cpu_set_errno(EINVAL);
        return NULL;
    }

    /* Handle absolute vs relative paths */
    if (path_name[0] != '/') {
        if (sys_get_full_path(VFS_FW_CWD, path_name, temp_buffer) == SYSCALL_FAIL) {
            kloge("'%s' is not a valid path!\n", path_name);
            cpu_set_errno(EINVAL);
            return NULL;
        }
        /* Skip leading slash from full path */
        if (strlen(temp_buffer) == 0 || temp_buffer[0] != '/') {
            kloge("Invalid full path generated for '%s'\n", path_name);
            cpu_set_errno(EINVAL);
            return NULL;
        }

        if (strlen(temp_buffer + 1) >= sizeof(normalized_path)) {
            kloge("Path too long: '%s'\n", temp_buffer);
            cpu_set_errno(ENAMETOOLONG);
            return NULL;
        }
        strcpy(normalized_path, temp_buffer + 1);
    } else {
        /* Skip the leading slash */
        if (strlen(path_name + 1) >= sizeof(normalized_path)) {
            kloge("Path too long: '%s'\n", path_name);
            cpu_set_errno(ENAMETOOLONG);
            return NULL;
        }
        strcpy(normalized_path, path_name + 1);
    }

    /* Normalize the path (resolve .. and . components) */
    if (normalize_path(normalized_path, normalized_path, sizeof(normalized_path)) < 0) {
        kloge("'%s' is an invalid path (attempt to escape root or path too long)\n", path_name);
        cpu_set_errno(EINVAL);
        return NULL;
    }

    size_t path_len = strlen(normalized_path);
    /* +1 for the leading slash that was removed */
    if (strlen(path_name) != path_len + 1) {
        klogd("VFS: \"%s\" -> \"/%s\"\n", path_name, normalized_path);
    }

    /* Handle root directory case */
    if (path_len == 0) {
        if ((mode & ERR_ON_EXIST)) {
            kloge("VFS: Root directory already exists\n");
            cpu_set_errno(EEXIST);
            return NULL;
        }
        return &vfs_root;
    }

    size_t current_index = 0;
    uint8_t found_node = TRUE;
    char *last_token = NULL;

    while (current_index < path_len) {
        size_t token_len = extract_path_token(normalized_path, current_index, path_len,
                                            token_buffer, sizeof(token_buffer));

        if (token_len == 0) {
            break;
        }

        if (token_len >= sizeof(token_buffer)) {
            kloge("Path component too long in '%s'\n", path_name);
            cpu_set_errno(ENAMETOOLONG);
            return NULL;
        }

        current_index += token_len;
        if (current_index < path_len && normalized_path[current_index] == '/') {
            /* Skip the slash */
            current_index++;
        }

        if (strcmp(token_buffer, ".") == 0) {
            continue;
        }

        last_token = token_buffer;

        /* Look for the token among current node's children */
        VFS_TNODE *child = find_child_node(current_node, token_buffer);
        if (child) {
            current_node = child;
            found_node = TRUE;
        } else {
            found_node = FALSE;
            break;
        }
    }

    if (!found_node) {
        /* Node not found; handle creation if specified */
        if ((mode & CREATE) && last_token && IS_TRAVERSABLE(current_node->inode)) {
            result = create_vfs_node(current_node, last_token, type);
            if (result) {
                for (size_t i = 0; i < num_log_paths; i++) {
                    if (strncmp(normalized_path, log_paths[i], strlen(log_paths[i])) == 0) {
                        klogd("VFS: Created \"%s\" node\n", normalized_path);
                        break;
                    }
                }
            }
            /* Error code already set by create_vfs_node if result is NULL */
        } else {
            kloge("VFS: \"%s\" doesn't exist\n", normalized_path);
            cpu_set_errno(ENOENT);
            result = NULL;
        }
    } else if (mode & ERR_ON_EXIST) {
        kloge("VFS: \"%s\" already exists\n", normalized_path);
        cpu_set_errno(EEXIST);
        result = NULL;
    } else {
        /* Node found without needing to create a new one */
        result = current_node;
    }

    return result;
}

/**
 * @brief Creates a new TNODE
 *
 * @param path Where the TNODE will be located at
 * @param type Node type
 * @return STATUS SYS_ERR if failure, SYS_OK if success
 */
STATUS vfs_create(char *path, VFS_NODE_TYPE type) {
    if (!path) {
        cpu_set_errno(EINVAL);
        return SYS_ERR;
    }

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
        cpu_set_errno(EACCES);
        return SYS_ERR;
    }

    /* Set new permissions */
    fd->inode->permissions = perms & (S_IRWXU | S_IRWXG | S_IRWXO);
    fd->tnode->stat.mode |= fd->inode->permissions;
    if (fd->inode->fs && fd->inode->fs->sync) {
        fd->inode->fs->sync(fd->inode);
    }
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

    if (fd->inode->fs && fd->inode->fs->ioctl) {
        return fd->inode->fs->ioctl(fd->inode, request, arg);
    }

    cpu_set_errno(ENOTTY);
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
    if (!path || !fs_name) {
        cpu_set_errno(EINVAL);
        return SYS_ERR;
    }

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
        if (!device) {
            cpu_set_errno(EINVAL);
            UNLOCK_LOCK(&vfs_lock);
            return SYS_ERR;
        }

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

    VFS_INODE *old_inode = at->inode;

    /* Now, mount the filesystem */
    if (!fs->mount) {
        cpu_set_errno(ENOTSUP);
        UNLOCK_LOCK(&vfs_lock);
        return SYS_ERR;
    }

    at->inode = fs->mount(dev ? dev->inode : NULL);
    if (!at->inode) {
        /* Restore original inode */
        at->inode = old_inode;
        cpu_set_errno(EIO);
        UNLOCK_LOCK(&vfs_lock);
        return SYS_ERR;
    }

    kfree(old_inode);
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
        cpu_set_errno(EBADF);
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

    if (!buff) {
        cpu_set_errno(EFAULT);
        return -1;
    }

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

    if (fd->mode == VFS_WRITE) {
        /* File not opened for reading */
        cpu_set_errno(EBADF);
        return -1;
    }

    LOCK_LOCK(&vfs_lock);

    VFS_INODE *inode = fd->inode;

    /* Truncate if asking for more data than available */
    if (fd->seek_position + len > inode->size &&
        strcmp(fd->inode->fs->name, "ttyfs") != 0 &&
        strcmp(fd->inode->fs->name, "pipefs") != 0) {
        len = inode->size - fd->seek_position;
        if (len == 0) {
            UNLOCK_LOCK(&vfs_lock);
            return (int64_t) len;
        }
    }

    int64_t ret = -1;
    if (fd->inode->fs && fd->inode->fs->read) {
        ret = fd->inode->fs->read(fd->inode, fd->seek_position, len, buff);
    } else {
        cpu_set_errno(ENOTSUP);
    }

    if (ret < 0) {
        /* Error occured */
        len = 0;
    } else {
        len = ret;
        fd->seek_position += len;
    }

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
    klogd("VFS: unlinking %s\n", path);

    if (!path) {
        cpu_set_errno(EINVAL);
        return -1;
    }

    LOCK_LOCK(&vfs_lock);

    /* Find the node and set the nlink parameter */
    VFS_TNODE *tnode = vfs_path_to_node(path, NO_CREATE, 0);
    if (!tnode) {
        kloge("VFS: Cannot find tnode for %s\n", path);
        /* Path doesn't relate to any tnode */
        cpu_set_errno(ENOENT);
        UNLOCK_LOCK(&vfs_lock);
        return -1;
    } else {
        if (tnode->inode->type == VFS_DIRECTORY) {
            kloge("VFS: Unlink: \"%s\" refers to a directory\n", path);
            cpu_set_errno(EISDIR);
            UNLOCK_LOCK(&vfs_lock);
            return -1;
        } else if (tnode->stat.nlink > 1) {
            kloge("VFS: Unlink: \"%s\" has links which should be removed first\n", path);
            cpu_set_errno(EMLINK);
            UNLOCK_LOCK(&vfs_lock);
            return -1;
        } else if (tnode->stat.nlink == 0) {
            kloge("VFS: Unlink: \"%s\" should only have one link, but has zero\n", path);
            cpu_set_errno(ENOENT);
            UNLOCK_LOCK(&vfs_lock);
            return -1;
        }

        tnode->stat.nlink = 0;
    }

    /* Remove the file if needed */
    if (tnode->inode->references == 0) {
        if (tnode->inode->fs && tnode->inode->fs->rmnode) {
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
 * @return int64_t Number of bytes written if success, -1 if failure
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
        cpu_set_errno(EBADF);
        return -1;
    }

    LOCK_LOCK(&vfs_lock);
    VFS_INODE *inode = fd->inode;

    /* Expand file if writing more data than its size */
    if (fd->seek_position + count > inode->size) {
        inode->size = fd->seek_position + count;
        if (inode->fs && inode->fs->sync) {
            inode->fs->sync(inode);
        }
    }

    int64_t ret = -1;
    if (inode->fs && inode->fs->write) {
        ret = inode->fs->write(inode, fd->seek_position, count, buf);
    } else {
        cpu_set_errno(ENOTSUP);
        UNLOCK_LOCK(&vfs_lock);
        return -1;
    }

    if (ret == -1) {
        count = 0;
    } else {
        /* Move seek position to tail of writing area */
        count = ret;
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
        if (fd->inode->fs && fd->inode->fs->sync) {
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
    if (pos >= 0 && pos <= (int64_t)fd->inode->size) {
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
 * @param parent Buffer to copy the parent directory into
 * @param curr_dir Buffer to copy the current (leaf) directory name into (optional)
 * @return int64_t -1 if failure, 0 if success
 */
int64_t vfs_get_parent_dir(const char *path, char *parent, char *curr_dir) {
    if (!path || !parent) {
        return -1;
    }

    /* Copy the original path into the parent buffer */
    strcpy(parent, path);

    size_t len = strlen(parent);
    if (len == 0) {
        return -1;
    }

    /* Remove trailing slashes (if any) */
    while (len > 1 && parent[len - 1] == '/') {
        parent[len - 1] = '\0';
        len--;
    }

    /* Find the last slash in the path */
    char *last_slash = strrchr(parent, '/');
    if (!last_slash) {
        /* No slash found means there's no parent directory */
        parent[0] = '\0';
        return -1;
    }

    /* If the last slash is at the beginning, the parent is "/" */
    if (last_slash == parent) {
        if (curr_dir) {
            strcpy(curr_dir, parent + 1);
        }
        /* Set parent to "/" */
        parent[1] = '\0';
        return 0;
    }

    /* Set curr_dir if provided */
    if (curr_dir) {
        strcpy(curr_dir, last_slash + 1);
    }

    /* Terminate the string at the last slash to remove the leaf component */
    *last_slash = '\0';

    /* In case the parent is empty after removal, set it to "/" */
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
    if (!path) {
        cpu_set_errno(EINVAL);
        return VFS_INVALID_HANDLE;
    }

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
                UNLOCK_LOCK(&vfs_lock);
                cpu_set_errno(ENOENT);
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

        if (tnode && tnode->inode->fs && tnode->inode->fs->open) {
            kloge("VFS: Open: Can not open %s, visit back to %s\n", path, parent);
            req = tnode->inode->fs->open(tnode->inode, path);
        }
        if (!req) {
            UNLOCK_LOCK(&vfs_lock);
            kloge("VFS: Open: Failed to open %s with mode %x\n", path, mode);
            cpu_set_errno(ENOENT);
            return VFS_INVALID_HANDLE;
        }
    } else {
        /* Move forward to open the file */
        if (req->inode->fs && req->inode->fs->open) {
            klogd("VFS: Open: inode for %s already exists\n", path);
            VFS_TNODE *opened_req = req->inode->fs->open(req->inode, path);
            if (opened_req) {
                req = opened_req;
            }
        }
    }

    req->inode->references++;

    /* Create node descriptor */
    VFS_NODE_DESC *nd = (VFS_NODE_DESC *)(kmalloc(sizeof(VFS_NODE_DESC)));
    if (!nd) {
        req->inode->references--;
        UNLOCK_LOCK(&vfs_lock);
        cpu_set_errno(ENOMEM);
        return VFS_INVALID_HANDLE;
    }

    memset(nd, 0, sizeof(VFS_NODE_DESC));

    size_t path_len = strlen(path);
    if (path_len >= sizeof(nd->path)) {
        path_len = sizeof(nd->path) - 1;
    }
    memcpy(nd->path, path, path_len);
    nd->path[path_len] = '\0';

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
        kfree(nd);
        req->inode->references--;
        UNLOCK_LOCK(&vfs_lock);
        cpu_set_errno(ESRCH);
        return VFS_INVALID_HANDLE;
    }

    UNLOCK_LOCK(&vfs_lock);

    klogd("VFS Open: Opened %s with mode %x and return handle %d, "
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
        cpu_set_errno(EBADF);
        return -1;
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
        if (nd->inode->fs && nd->inode->fs->rmnode) {
            klogd("VFS: Close: close \"%s\" and remove tnode\n", nd->path);
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
        cpu_set_errno(EBADF);
        return -1;
    }

    LOCK_LOCK(&vfs_lock);

    if (nd->inode->fs && nd->inode->fs->refresh) {
        nd->inode->fs->refresh(nd->inode);
    }

    if (nd->inode->fs && nd->inode->fs->getdent) {
        for (size_t i = 0;; i++) {
            VFS_DIR_ENTRY de;
            if (nd->inode->fs->getdent(nd->inode, i, &de)) {
                break;
            }

            char path[VFS_MAX_PATH_LEN] = {0};
            strcpy(path, nd->path);
            strcat(path, "/");
            strncat(path, de.name, sizeof(path) - strlen(path) - 1);

            VFS_TNODE *tnode = vfs_path_to_node(path, CREATE, de.type);
            if (tnode) {
                memcpy(&tnode->inode->time, &de.time, sizeof(STD_TIME));
                tnode->inode->size = de.size;
            }
        }
    }

    UNLOCK_LOCK(&vfs_lock);

    return 0;
}

/**
 * @brief Get directory entry at current position
 *
 * @param h Handle to directory
 * @param dirent Directory entry structure to fill
 * @return int64_t 0 if success, -1 if failure or end of directory
 */
int64_t vfs_getdent(VFS_HANDLE h, VFS_DIR_ENTRY *dirent) {
    if (!dirent) {
        cpu_set_errno(EFAULT);
        return -1;
    }

    VFS_NODE_DESC *nd = vfs_handle_to_fd(h);
    if (!nd) {
        cpu_set_errno(EBADF);
        return -1;
    }

    LOCK_LOCK(&vfs_lock);

    /* Can only traverse folders */
    if (!IS_TRAVERSABLE(nd->inode)) {
        kloge("VFS: getdent: Node not traversable\n");
        UNLOCK_LOCK(&vfs_lock);
        cpu_set_errno(ENOTDIR);
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
    if (!entry) {
        UNLOCK_LOCK(&vfs_lock);
        cpu_set_errno(EIO);
        return -1;
    }

    dirent->type = entry->inode->type;

    size_t name_len = strlen(entry->name);
    if (name_len >= sizeof(dirent->name)) {
        name_len = sizeof(dirent->name) - 1;
    }
    memcpy(dirent->name, entry->name, name_len);
    dirent->name[name_len] = '\0';

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
    if (!vfs_root.inode) {
        kloge("INIT VFS: Failed to allocate root inode!\n");
        return;
    }

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
    if (vfs_mount(NULL, "/", "ramfs") != SYS_OK) {
        kloge("INIT VFS: Failed to mount ramfs at root!\n");
        return;
    }

    /* Create directory for mounting devices */
    if (vfs_path_to_node("/disk", CREATE, VFS_DIRECTORY) == NULL) {
        kloge("INIT VFS: Failed to create /disk directory!\n");
    }

    if (vfs_path_to_node("/dev", CREATE, VFS_DIRECTORY) == NULL) {
        kloge("INIT VFS: Failed to create /dev directory!\n");
    }

    /* Mount TTYFS with device name "/dev/tty" */
    if (vfs_path_to_node("/dev/tty", CREATE, VFS_DIRECTORY) == NULL) {
        kloge("INIT VFS: Failed to create /dev/tty directory!\n");
    } else if (vfs_mount("tty", "/dev/tty", "ttyfs") != SYS_OK) {
        kloge("INIT VFS: Failed to mount ttyfs at /dev/tty!\n");
    }

    /* Mount PIPEFS with device name "/dev/pipe" */
    if (vfs_path_to_node("/dev/pipe", CREATE, VFS_DIRECTORY) == NULL) {
        kloge("INIT VFS: Failed to create /dev/pipe directory!\n");
    } else if (vfs_mount("pipe", "/dev/pipe", "pipefs") != SYS_OK) {
        kloge("INIT VFS: Failed to mount pipefs at /dev/pipe!\n");
    }

    klogs("INIT VFS: finished...\n");
}