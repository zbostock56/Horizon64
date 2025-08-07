/**
 * @file ramfs.c
 * @author Zack Bostock
 * @brief RAM filesystem implementation with initrd support
 * @ref https://wiki.osdev.org/Initrd
 *
 * @copyright Copyright (c) 2025
 */

#include <common/time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <common/kmalloc.h>
#include <common/vector.h>
#include <common/math.h>
#include <common/kprint.h>

#include <sys/asm.h>

#include <fs/ramfs.h>
#include <fs/vfs.h>

#define RAMFS_USTAR_MAGIC "ustar"
#define RAMFS_USTAR_MAGIC_LEN 5
#define RAMFS_USTAR_MAGIC_OFFSET 257
#define RAMFS_BLOCK_SIZE 512
#define RAMFS_MAX_RETRY_COUNT 3

typedef enum {
    RAMFS_SUCCESS = 0,
    RAMFS_ERROR_MEMORY = -1,
    RAMFS_ERROR_INVALID_PATH = -2,
    RAMFS_ERROR_NOT_FOUND = -3,
    RAMFS_ERROR_INVALID_TYPE = -4,
    RAMFS_ERROR_IO = -5
} ramfs_error_t;

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
 * @brief Create and initialize a new RAMFS identifier
 * @return RAMFS_IDENT* New identifier, NULL on failure
 */
static RAMFS_IDENT *ramfs_create_ident(void) {
    RAMFS_IDENT *ident = kmalloc(sizeof(RAMFS_IDENT));
    if (!ident) {
        kloge("RAMFS: Failed to allocate memory for identifier\n");
        return NULL;
    }

    ident->alloc_size = 0;
    ident->data = NULL;
    return ident;
}

/**
 * @brief Clean up RAMFS identifier and free associated memory
 * @param ident Identifier to clean up
 * @return ramfs_error_t Success or error code
 */
static ramfs_error_t ramfs_cleanup_ident(RAMFS_IDENT *ident) {
    if (!ident) {
        return RAMFS_SUCCESS;
    }

    if (ident->data) {
        kfree(ident->data);
        ident->data = NULL;
    }
    ident->alloc_size = 0;

    return RAMFS_SUCCESS;
}

/**
 * @brief Convert octal string to binary (from OSDev Wiki)
 * @param str Octal string to convert
 * @param size Number of bytes to process
 * @return uint32_t Converted binary value
 */
static uint32_t ramfs_oct2bin(const unsigned char *str, size_t size) {
    if (!str || size == 0) {
        return 0;
    }

    uint32_t result = 0;
    const unsigned char *ptr = str;

    while (size-- > 0 && *ptr >= '0' && *ptr <= '7') {
        result = (result << 3) + (*ptr - '0');
        ptr++;
    }

    return result;
}

/**
 * @brief Convert USTAR file type to VFS file type
 * @param ustar_type USTAR type character
 * @return uint8_t Corresponding VFS type
 */
static uint8_t ramfs_ustar_to_vfs_type(uint8_t ustar_type) {
    switch (ustar_type) {
        case '0':
        /* Some tar implementations use null for regular files */
        case '\0':
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
            klogw("RAMFS: Unknown USTAR type '%c' (0x%02x), treating as invalid\n",
                  ustar_type, ustar_type);
            return VFS_INVALID;
    }
}

/**
 * @brief Validate path format and length
 * @param path Path to validate
 * @return ramfs_error_t Success or error code
 */
static ramfs_error_t ramfs_validate_path(const char *path) {
    if (!path) {
        return RAMFS_ERROR_INVALID_PATH;
    }

    size_t len = strlen(path);
    if (len == 0 || len >= VFS_MAX_PATH_LEN) {
        return RAMFS_ERROR_INVALID_PATH;
    }

    if (path[0] != '/') {
        return RAMFS_ERROR_INVALID_PATH;
    }

    return RAMFS_SUCCESS;
}

/**
 * @brief Normalize path by resolving .. and . components
 * @param input Input path
 * @param output Output buffer for normalized path
 * @param output_size Size of output buffer
 * @return ramfs_error_t Success or error code
 */
static ramfs_error_t ramfs_normalize_path(const char *input, char *output, size_t output_size) {
    if (!input || !output || output_size == 0) {
        return RAMFS_ERROR_INVALID_PATH;
    }

    if (ramfs_validate_path(input) != RAMFS_SUCCESS) {
        return RAMFS_ERROR_INVALID_PATH;
    }

    strncpy(output, input, output_size - 1);
    output[output_size - 1] = '\0';

    size_t len = strlen(output);

    /* Remove the trailing slash if not root */
    if (len > 1 && output[len - 1] == '/') {
        output[len - 1] = '\0';
        len--;
    }

    /* Resolve the '..' sequences */
    for (size_t i = 0; i + 3 < len; i++) {
        if (output[i] == '/' && output[i + 1] == '.' &&
            output[i + 2] == '.' && output[i + 3] == '/') {

            /* Find the parent directory */
            int64_t parent_start = -1;
            for (int64_t j = i - 1; j >= 0; j--) {
                if (output[j] == '/') {
                    parent_start = j;
                    break;
                }
            }

            if (parent_start >= 0) {
                /* Move remaining string over the parent/.. sequence */
                memmove(&output[parent_start], &output[i + 3], len - (i + 3) + 1);
                len = strlen(output);
                /* Now, restart from beginning */
                i = 0;
            } else {
                return RAMFS_ERROR_INVALID_PATH;
            }
        }
    }

    return RAMFS_SUCCESS;
}

/**
 * @brief Find RAMFS item by path
 * @param path Path to search for
 * @return RAMFS_IDENT_ITEM* Found item or NULL
 */
static RAMFS_IDENT_ITEM* ramfs_find_item_by_path(const char *path) {
    if (!path) {
        return NULL;
    }

    for (size_t i = 0; i < vector_len(&ramfs.files); i++) {
        RAMFS_IDENT_ITEM *item = vector_at(&ramfs.files, i);
        if (item && strcmp(item->path, path) == 0) {
            return item;
        }
    }

    return NULL;
}

/**
 * @brief Create and initialize file item from USTAR entry
 * @param file USTAR file structure
 * @param full_path Full path for the file
 * @param filesize Size of file data
 * @return ramfs_error_t Success or error code
 */
static ramfs_error_t ramfs_create_file_item(USTAR_FILE *file, const char *full_path, uint32_t filesize) {
    if (!file || !full_path) {
        return RAMFS_ERROR_INVALID_PATH;
    }

    RAMFS_IDENT_ITEM *item = kmalloc(sizeof(RAMFS_IDENT_ITEM));
    if (!item) {
        kloge("RAMFS: Failed to allocate memory for file item\n");
        return RAMFS_ERROR_MEMORY;
    }

    memset(item, 0, sizeof(RAMFS_IDENT_ITEM));

    /* Extract file name from the full path */
    const char *filename = strrchr(full_path, '/');
    if (filename) {
        /* Skip '/' */
        filename++; // Skip the '/'
    } else {
        filename = full_path;
    }

    strncpy(item->name, filename, sizeof(item->name) - 1);
    strncpy(item->path, full_path, sizeof(item->path) - 1);

    item->type = ramfs_ustar_to_vfs_type(file->type);
    item->entry.size = filesize;

    uint64_t file_time = strtol((char*)file->last_modified, NULL, 8);
    seconds_to_std_time(file_time, &item->time);

    if (item->type == VFS_SYMLINK) {
        size_t link_size = strnlen((const char *)file->linked_file_name,
                                    sizeof(file->linked_file_name));
        if (link_size > 0) {
            item->entry.data = kmalloc(link_size + 1);
            if (!item->entry.data) {
                kfree(item);
                return RAMFS_ERROR_MEMORY;
            }
            memcpy(item->entry.data, file->linked_file_name, link_size);
            ((char*)item->entry.data)[link_size] = '\0';
            item->entry.size = link_size;
        }
    } else if (filesize > 0) {
        item->entry.data = kmalloc(filesize);
        if (!item->entry.data) {
            kfree(item);
            return RAMFS_ERROR_MEMORY;
        }
        /* Data will be copied from caller to archive */
    }

    /* Add files to ramfs open file list */
    if (vector_append(&ramfs.files, item) != 0) {
        if (item->entry.data) {
            kfree(item->entry.data);
        }
        kfree(item);
        return RAMFS_ERROR_MEMORY;
    }

    return RAMFS_SUCCESS;
}

/**
 * @brief Initialize RAMFS from initrd archive
 * @param addr Address of initrd archive
 * @param size Size of initrd archive
 */
void init_ramfs(void *addr, uint64_t size) {
    if (!addr || size == 0) {
        kloge("RAMFS: Invalid initrd parameters\n");
        return;
    }

    klogs("RAMFS: Initializing from initrd at %x (size: %d bytes)\n", addr, size);

    unsigned char *ptr = (unsigned char*)addr;
    unsigned char *end = ptr + size;
    size_t files_processed = 0;

    /* Process USTAR archive */
    while (ptr + RAMFS_BLOCK_SIZE <= end) {
        /* Check for USTAR MAGIC */
        if (memcmp(ptr + RAMFS_USTAR_MAGIC_OFFSET, RAMFS_USTAR_MAGIC,
                    RAMFS_USTAR_MAGIC_LEN) != 0) {
            /* Check if we've hit padding/end of archive */
            int is_empty = 1;
            for (size_t i = 0; i < RAMFS_BLOCK_SIZE && is_empty; i++) {
                if (ptr[i] != 0) {
                    is_empty = 0;
                }
            }
            if (is_empty) {
                /* End of archive */
                break;
            }
            ptr += RAMFS_BLOCK_SIZE;
            continue;
        }

        USTAR_FILE *file = (USTAR_FILE*)ptr;
        uint32_t filesize = ramfs_oct2bin(ptr + 0x7c, 11);
        uint8_t vfs_type = ramfs_ustar_to_vfs_type(file->type);

        if (vfs_type == VFS_INVALID) {
            klogw("RAMFS: Skipping file '%s' with invalid type\n", file->name);
            ptr += RAMFS_BLOCK_SIZE + DIV_ROUNDUP(filesize, RAMFS_BLOCK_SIZE) * RAMFS_BLOCK_SIZE;
            continue;
        }

        /* Build full path */
        char full_path[VFS_MAX_PATH_LEN];
        if (file->name[0] == '/') {
            strncpy(full_path, file->name, sizeof(full_path) - 1);
        } else {
            snprintf(full_path, sizeof(full_path), "/%s", file->name);
        }
        full_path[sizeof(full_path) - 1] = '\0';

        /* Remove trailing slash for directories (except root dir) */
        size_t path_len = strlen(full_path);
        if (path_len > 1 && full_path[path_len - 1] == '/') {
            full_path[path_len - 1] = '\0';
        }

        VFS_TNODE *tnode = NULL;

        switch (vfs_type) {
            case VFS_DIRECTORY: {
                tnode = vfs_path_to_node(full_path, CREATE, VFS_DIRECTORY);
                if (!tnode) {
                    kloge("RAMFS: Failed to create directory node for '%s'\n", full_path);
                    break;
                }

                // Set permissions and timestamps
                /* TODO: Fix permissions on ramfs USTAR_FILE * */
                /* tnode->inode->permissions = */
                /*    ramfs_oct2bin((unsigned char*)file->mode, 7) & 0777; */
                tnode->inode->permissions = 0777;
                tnode->stat.mode |= tnode->inode->permissions;

                uint64_t file_time = strtol((char*)file->last_modified, NULL, 8);
                tnode->stat.access_time.TV_SEC = file_time;
                tnode->stat.modify_time.TV_SEC = file_time;
                tnode->stat.status_change_time.TV_SEC = file_time;
                tnode->stat.access_time.TV_NSEC = 0;
                tnode->stat.modify_time.TV_NSEC = 0;
                tnode->stat.status_change_time.TV_NSEC = 0;
                tnode->stat.nlink = 1;
                break;
            }

            case VFS_FILE:
            case VFS_SYMLINK: {
                ramfs_error_t result = ramfs_create_file_item(file, full_path, filesize);
                if (result != RAMFS_SUCCESS) {
                    kloge("RAMFS: Failed to create file item for '%s'\n", full_path);
                    break;
                }

                tnode = vfs_path_to_node(full_path, CREATE, vfs_type);
                if (!tnode) {
                    kloge("RAMFS: Failed to create VFS node for '%s'\n", full_path);
                    break;
                }

                /* Set up inode */
                /* TODO: Fix permissions on ramfs USTAR_FILE * */
                /* tnode->inode->permissions = */
                /*      ramfs_oct2bin((unsigned char*)file->mode, 7) & 0777; */
                tnode->inode->permissions = 0777;
                tnode->stat.mode |= tnode->inode->permissions;

                uint64_t file_time = strtol((char*)file->last_modified, NULL, 8);
                tnode->stat.access_time.TV_SEC = file_time;
                tnode->stat.modify_time.TV_SEC = file_time;
                tnode->stat.status_change_time.TV_SEC = file_time;
                tnode->stat.access_time.TV_NSEC = 0;
                tnode->stat.modify_time.TV_NSEC = 0;
                tnode->stat.status_change_time.TV_NSEC = 0;
                tnode->stat.nlink = 1;

                if (vfs_type == VFS_FILE) {
                    tnode->inode->size = filesize;
                    tnode->stat.size = filesize;

                    /* Copy file data if present */
                    if (filesize > 0) {
                        RAMFS_IDENT_ITEM *item = ramfs_find_item_by_path(full_path);
                        if (item && item->entry.data) {
                            memcpy(item->entry.data, ptr + RAMFS_BLOCK_SIZE, filesize);
                        }
                    }
                } else {
                    /* Symlink */
                    tnode->inode->size = strnlen((const char *)file->linked_file_name,
                                                  sizeof(file->linked_file_name));
                    if (tnode->inode->size <= sizeof(tnode->inode->symlink)) {
                        memcpy(tnode->inode->symlink, file->linked_file_name,
                               tnode->inode->size);
                    }
                    /* Symlinks are set to size of 0 in stat */
                    tnode->stat.size = 0;
                }

                /* Set up parent relationship */
                char parent_path[VFS_MAX_PATH_LEN];
                strncpy(parent_path, full_path, sizeof(parent_path) - 1);
                parent_path[sizeof(parent_path) - 1] = '\0';

                char *last_slash = strrchr(parent_path, '/');
                if (last_slash && last_slash != parent_path) {
                    *last_slash = '\0';
                    VFS_TNODE *parent_tnode = vfs_path_to_node(parent_path, NO_CREATE, 0);
                    if (parent_tnode) {
                        RAMFS_IDENT_ITEM *item = ramfs_find_item_by_path(full_path);
                        if (item) {
                            item->parent = parent_tnode->inode;
                        }
                        tnode->parent = parent_tnode->inode;
                    }
                }
                break;
            }

            default:
                klogw("RAMFS: Unsupported file type %d for '%s'\n", vfs_type, full_path);
                break;
        }

        files_processed++;
        ptr += RAMFS_BLOCK_SIZE + DIV_ROUNDUP(filesize, RAMFS_BLOCK_SIZE) * RAMFS_BLOCK_SIZE;
    }

    klogs("RAMFS: Initialization complete, processed %d files\n", files_processed);
}

/**
 * @brief Handle symlink resolution
 * @param ident RAMFS identifier
 * @param path Original path
 * @param resolved_path Buffer for resolved path
 * @param resolved_size Size of resolved path buffer
 * @return ramfs_error_t Success or error code
 */
static ramfs_error_t ramfs_handle_symlink(RAMFS_IDENT *ident, const char *path,
                                         char *resolved_path, size_t resolved_size) {
    if (!ident || !path || !resolved_path || resolved_size == 0) {
        return RAMFS_ERROR_INVALID_PATH;
    }

    RAMFS_IDENT_ITEM *item = ramfs_find_item_by_path(path);
    if (!item || item->type != VFS_SYMLINK || !item->entry.data) {
        return RAMFS_ERROR_NOT_FOUND;
    }

    const char *link_target = (const char*)item->entry.data;

    if (link_target[0] == '/') {
        /* Absolute path */
        strncpy(resolved_path, link_target, resolved_size - 1);
    } else {
        /* Relative path - resolve relative to symlink's directory */
        strncpy(resolved_path, path, resolved_size - 1);
        char *last_slash = strrchr(resolved_path, '/');
        if (last_slash) {
            *(last_slash + 1) = '\0';
            strncat(resolved_path, link_target, resolved_size - strlen(resolved_path) - 1);
        } else {
            return RAMFS_ERROR_INVALID_PATH;
        }
    }
    resolved_path[resolved_size - 1] = '\0';

    return RAMFS_SUCCESS;
}

/**
 * @brief Open file in RAMFS
 * @param this Inode to open
 * @param path Full path to file
 * @return VFS_TNODE* Opened node or NULL on failure
 */
VFS_TNODE *ramfs_open(VFS_INODE *this, const char *path) {
    if (!this || !path) {
        kloge("RAMFS: Invalid parameters for open\n");
        return NULL;
    }

    char normalized_path[VFS_MAX_PATH_LEN];
    if (ramfs_normalize_path(path, normalized_path, sizeof(normalized_path)) != RAMFS_SUCCESS) {
        kloge("RAMFS: Invalid path '%s'\n", path);
        return NULL;
    }

    RAMFS_IDENT *ident = (RAMFS_IDENT*)this->ident;
    if (!ident) {
        ident = ramfs_create_ident();
        if (!ident) {
            return NULL;
        }
        this->ident = ident;
    }

    /* Handle symlink resolution */
    char final_path[VFS_MAX_PATH_LEN];
    strncpy(final_path, normalized_path, sizeof(final_path) - 1);
    final_path[sizeof(final_path) - 1] = '\0';

    RAMFS_IDENT_ITEM *item = ramfs_find_item_by_path(final_path);
    if (item && item->type == VFS_SYMLINK) {
        char resolved_path[VFS_MAX_PATH_LEN];
        if (ramfs_handle_symlink(ident, final_path, resolved_path, sizeof(resolved_path)) == RAMFS_SUCCESS) {
            strncpy(final_path, resolved_path, sizeof(final_path) - 1);
            final_path[sizeof(final_path) - 1] = '\0';
            item = ramfs_find_item_by_path(final_path);
        }
    }

    /* Load file data */
    if (item && item->entry.size > 0) {
        /* Clean up existing data, if any is there */
        ramfs_cleanup_ident(ident);

        ident->data = kmalloc(item->entry.size);
        if (!ident->data) {
            kloge("RAMFS: Failed to allocate memory for file data\n");
            return NULL;
        }

        ident->alloc_size = item->entry.size;
        if (item->entry.data) {
            memcpy(ident->data, item->entry.data, item->entry.size);
        }
    } else if (item && item->entry.size == 0) {
        /* This file is empty */
        ramfs_cleanup_ident(ident);
        ident->data = NULL;
        ident->alloc_size = 0;
    }

    VFS_TNODE *tnode = vfs_path_to_node(normalized_path, NO_CREATE, 0);
    if (tnode && item && item->type != VFS_SYMLINK) {
        tnode->inode->size = ident->alloc_size;
    }

    return tnode;
}

/**
 * @brief Read data from RAMFS file
 * @param this Inode to read from
 * @param offset Offset to start reading
 * @param len Number of bytes to read
 * @param buff Buffer to read into
 * @return int64_t Number of bytes read, or negative error code
 */
int64_t ramfs_read(VFS_INODE *this, size_t offset, size_t len, void *buff) {
    if (!this || !buff) {
        return RAMFS_ERROR_INVALID_PATH;
    }

    RAMFS_IDENT *ident = (RAMFS_IDENT*)this->ident;
    if (!ident) {
        /* This file is empty */
        return 0;
    }

    size_t read_len = len;
    if (offset >= ident->alloc_size) {
        /* Read past the end of the file */
        return 0;
    }

    if (offset + read_len > ident->alloc_size) {
        read_len = ident->alloc_size - offset;
    }

    if (read_len > 0 && ident->data) {
        memcpy(buff, ((uint8_t*)ident->data) + offset, read_len);
    }

    return read_len;
}

/**
 * @brief Write data to RAMFS file
 * @param this Inode to write to
 * @param offset Offset to start writing
 * @param len Number of bytes to write
 * @param buff Buffer to write from
 * @return int64_t Number of bytes written, or negative error code
 */
int64_t ramfs_write(VFS_INODE *this, size_t offset, size_t len, const void *buff) {
    if (!this || !buff) {
        return RAMFS_ERROR_INVALID_PATH;
    }

    RAMFS_IDENT *ident = (RAMFS_IDENT*)this->ident;
    if (!ident) {
        ident = ramfs_create_ident();
        if (!ident) {
            return RAMFS_ERROR_MEMORY;
        }
        this->ident = ident;
    }

    /* Update file size, if needed */
    size_t new_size = offset + len;
    if (new_size > this->size) {
        this->size = new_size;
    }

    /* Expand buffer, if needed */
    if (new_size > ident->alloc_size) {
        void *new_data = krealloc(ident->data, new_size);
        if (!new_data) {
            return RAMFS_ERROR_MEMORY;
        }

        /* Zero out new space in buffer */
        if (new_size > ident->alloc_size) {
            memset((uint8_t*)new_data + ident->alloc_size, 0, new_size - ident->alloc_size);
        }

        ident->data = new_data;
        ident->alloc_size = new_size;
    }

    /* Copy new data */
    memcpy(((uint8_t*)ident->data) + offset, buff, len);
    return len;
}

/**
 * @brief Synchronize RAMFS file
 * @param this Inode to sync
 * @return int64_t 0 on success, negative error code on failure
 */
int64_t ramfs_sync(VFS_INODE *this) {
    if (!this) {
        return RAMFS_ERROR_INVALID_PATH;
    }

    RAMFS_IDENT *ident = (RAMFS_IDENT*)this->ident;
    if (!ident) {
        /* Nothing to sync */
        return RAMFS_SUCCESS;
    }

    /* Ensure buffer size matches file size */
    if (this->size > ident->alloc_size) {
        void *new_data = krealloc(ident->data, this->size);
        if (!new_data && this->size > 0) {
            return RAMFS_ERROR_MEMORY;
        }

        if (this->size > ident->alloc_size) {
            memset((uint8_t*)new_data + ident->alloc_size, 0, this->size - ident->alloc_size);
        }

        ident->data = new_data;
        ident->alloc_size = this->size;
    }

    return RAMFS_SUCCESS;
}

/**
 * @brief Refresh RAMFS inode
 * @param this Inode to refresh
 * @return int64_t 0 on success, negative error code on failure
 */
int64_t ramfs_refresh(VFS_INODE *this) {
    (void)this;
    return RAMFS_SUCCESS;
}

/**
 * @brief Get directory entry from RAMFS
 * @param this Directory inode
 * @param pos Position in directory
 * @param dirent Directory entry structure to fill
 * @return int64_t 0 on success, negative error code on failure
 */
int64_t ramfs_getdent(VFS_INODE *this, size_t pos, VFS_DIR_ENTRY *dirent) {
    if (!this || !dirent) {
        return RAMFS_ERROR_INVALID_PATH;
    }

    size_t current_pos = 0;
    for (size_t i = 0; i < vector_len(&ramfs.files); i++) {
        RAMFS_IDENT_ITEM *item = vector_at(&ramfs.files, i);
        if (!item || (uint64_t)item->parent != (uint64_t)this) {
            continue;
        }

        if (current_pos == pos) {
            strncpy(dirent->name, item->name, sizeof(dirent->name) - 1);
            dirent->name[sizeof(dirent->name) - 1] = '\0';
            memcpy(&dirent->time, &item->time, sizeof(STD_TIME));
            dirent->type = item->type;
            dirent->size = item->entry.size;
            return RAMFS_SUCCESS;
        }
        current_pos++;
    }

    return RAMFS_ERROR_NOT_FOUND;
}

/**
 * @brief Create new node in RAMFS
 * @param this Node to create
 * @return int64_t 0 on success, negative error code on failure
 */
int64_t ramfs_mknode(VFS_TNODE *this) {
    if (!this || !this->inode) {
        return RAMFS_ERROR_INVALID_PATH;
    }

    RAMFS_IDENT *ident = ramfs_create_ident();
    if (!ident) {
        return RAMFS_ERROR_MEMORY;
    }

    this->inode->ident = ident;
    return RAMFS_SUCCESS;
}

/**
 * @brief Remove node from RAMFS
 * @param this Node to remove
 * @return int64_t 0 on success, negative error code on failure
 */
int64_t ramfs_rmnode(VFS_TNODE *this) {
    if (!this || !this->inode) {
        return RAMFS_ERROR_INVALID_PATH;
    }

    /* Remove from files vector */
    for (size_t i = 0; i < vector_len(&ramfs.files); i++) {
        RAMFS_IDENT_ITEM *item = vector_at(&ramfs.files, i);
        if (!item) {
            continue;
        }

        /* Check if this istem corresponds to the node being removed */
        if ((uint64_t)item->parent == (uint64_t)this->parent) {
            if (item->entry.data) {
                kfree(item->entry.data);
            }

            vector_erase(&ramfs.files, i);
            kfree(item);
            break;
        }
    }

    /* Remove inode identifier */
    if (this->inode->ident) {
        ramfs_cleanup_ident((RAMFS_IDENT*)this->inode->ident);
        kfree(this->inode->ident);
        this->inode->ident = NULL;
    }

    return RAMFS_SUCCESS;
}

/**
 * @brief Mount RAMFS filesystem
 * @param at Mount point inode
 * @return VFS_INODE* New mount point inode, NULL on failure
 */
VFS_INODE *ramfs_mount(VFS_INODE *at) {
    klogi("RAMFS: Mounting filesystem at inode %x\n", (void*)at);

    VFS_INODE *mount_inode = vfs_alloc_inode(VFS_MOUNT_POINT, 0755, 0, &ramfs, NULL);
    if (!mount_inode) {
        kloge("RAMFS: Failed to allocate mount point inode\n");
        return NULL;
    }

    RAMFS_IDENT *ident = ramfs_create_ident();
    if (!ident) {
        kfree(mount_inode);
        return NULL;
    }

    mount_inode->ident = ident;
    klogs("RAMFS: Successfully mounted filesystem\n");
    return mount_inode;
}