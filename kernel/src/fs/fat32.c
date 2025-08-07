/**
 * @file fat32.c
 * @author Zack Bostock (improved)
 * @brief Functionality related to FAT32 filesystem
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <stdint.h>

#include <common/kmalloc.h>
#include <common/vector.h>
#include <common/math.h>
#include <common/kprint.h>
#include <string.h>
#include <ctype.h>

#include <sys/cpu.h>
#include <sys/asm.h>

#include <fs/vfs.h>
#include <fs/fat32.h>

#include <dev/storage/ata.h>

/* Cache structure for improved performance */
typedef struct {
    uint32_t sector;
    uint8_t data[512];
    int dirty;
    uint64_t access_time;
} CACHE_ENTRY;

#define CACHE_SIZE 32
static CACHE_ENTRY sector_cache[CACHE_SIZE];
static uint64_t cache_access_counter = 0;

VFS_FS fat32 = {
    .name = "fat32",
    .is_temp = 0,
    .files = {0},
    .open = fat32_open,
    .mount = fat32_mount,
    .mknode = fat32_mknode,
    .rmnode = fat32_rmnode,
    .sync = fat32_sync,
    .refresh = fat32_refresh,
    .read = fat32_read,
    .getdent = fat32_getdent,
    .write = fat32_write,
    .ioctl = NULL
};

/**
 * @brief Create a FAT32 ident object
 *
 * @return FAT32_IDENT* New ident
 */
static FAT32_IDENT *create_ident() {
    FAT32_IDENT *id = (FAT32_IDENT *)(kmalloc(sizeof(FAT32_IDENT)));
    if (!id) {
        kloge("Failed to create a new FAT32_IDENT!\n");
        halt();
    }
    memset(id, 0, sizeof(FAT32_IDENT));
    return id;
}

/**
 * @brief Cache-aware sector read
 *
 * @param device ATA device
 * @param sector Sector number
 * @param buffer Buffer to read into
 * @return int 0 on success, -1 on failure
 */
static int cached_sector_read(ATA_DEVICE *device, uint32_t sector, uint8_t *buffer) {
    /* Check cache first */
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (sector_cache[i].sector == sector && sector_cache[i].data[0] != 0) {
            memcpy(buffer, sector_cache[i].data, 512);
            sector_cache[i].access_time = ++cache_access_counter;
            return 0;
        }
    }

    /* Cache miss - find LRU entry */
    int lru_idx = 0;
    uint64_t oldest_time = sector_cache[0].access_time;
    for (int i = 1; i < CACHE_SIZE; i++) {
        if (sector_cache[i].access_time < oldest_time) {
            oldest_time = sector_cache[i].access_time;
            lru_idx = i;
        }
    }

    /* Write back if dirty */
    if (sector_cache[lru_idx].dirty) {
        ata_pio_write28(device, sector_cache[lru_idx].sector, 1,
                       sector_cache[lru_idx].data);
        sector_cache[lru_idx].dirty = 0;
    }

    /* Read new sector */
    if (ata_pio_read28(device, sector, 1, buffer) != 0) {
        return -1;
    }

    /* Update cache */
    sector_cache[lru_idx].sector = sector;
    memcpy(sector_cache[lru_idx].data, buffer, 512);
    sector_cache[lru_idx].access_time = ++cache_access_counter;
    sector_cache[lru_idx].dirty = 0;

    return 0;
}

/**
 * @brief Cache-aware sector write
 *
 * @param device ATA device
 * @param sector Sector number
 * @param buffer Buffer to write from
 * @return int 0 on success, -1 on failure
 */
static int cached_sector_write(ATA_DEVICE *device, uint32_t sector, const uint8_t *buffer) {
    /* Update cache if present */
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (sector_cache[i].sector == sector && sector_cache[i].data[0] != 0) {
            memcpy(sector_cache[i].data, buffer, 512);
            sector_cache[i].dirty = 1;
            sector_cache[i].access_time = ++cache_access_counter;
            return ata_pio_write28(device, sector, 1, buffer);
        }
    }

    return ata_pio_write28(device, sector, 1, buffer);
}

/**
 * @brief Validate cluster number
 *
 * @param cluster Cluster to validate
 * @param id FAT32 identifier
 * @return int 1 if valid, 0 if invalid
 */
static int is_valid_cluster(uint32_t cluster, FAT32_IDENT *id) {
    if (cluster < 2) return 0;
    if (cluster >= 0x0FFFFFF8) return 0;

    uint32_t max_cluster = (id->bs.total_sectors - id->bs.cluster_begin_lba +
                           id->bs.fat_begin_lba) / id->bs.sectors_per_cluster + 2;
    return cluster < max_cluster;
}

/**
 * @brief Helper to read an entry with bounds checking
 *
 * @param this inode which is related to this entry
 * @param cluster Cluster number
 * @param index Index
 * @param dest Destination buffer to copy data to
 * @return FAT32_STATUS 0 if success, 1 if failure
 */
FAT32_STATUS fat32_read_entry(VFS_INODE* this, uint32_t cluster, size_t index,
                              FAT32_ENTRY *dest) {
    if (!this || !dest) return FAT32_ERR;

    FAT32_IDENT *id = (FAT32_IDENT *)(this->ident);
    if (!id) return FAT32_ERR;

    /* Validate cluster */
    if (!is_valid_cluster(cluster, id)) {
        return FAT32_ERR;
    }

    /* Calculate entries per cluster */
    size_t entries_per_cluster = (id->bs.sectors_per_cluster * id->bs.bytes_per_sector) /
                                sizeof(FAT_DIR_ENTRY);
    if (index >= entries_per_cluster) {
        return FAT32_ERR;
    }

    uint8_t dd[512] = {0};
    uint32_t sector = id->bs.cluster_begin_lba + (cluster - 2) * id->bs.sectors_per_cluster;
    sector += (index * sizeof(FAT_DIR_ENTRY)) / id->bs.bytes_per_sector;

    if (cached_sector_read(id->device, sector, dd) != 0) {
        return FAT32_ERR;
    }

    size_t entry_offset = (index * sizeof(FAT_DIR_ENTRY)) % id->bs.bytes_per_sector;
    FAT_DIR_ENTRY *de = (FAT_DIR_ENTRY *)(dd + entry_offset);

    /* Validate entry */
    if (de->name[0] == 0x00) {
        return FAT32_ERR; /* End of directory */
    }

    memcpy(dest->name, de->name, 11);
    dest->attribute = de->attributes;
    dest->cluster_begin = ((uint32_t)de->first_cluster_high << 16) |
                           de->first_cluster_low;
    dest->file_size_bytes = de->size;
    dest->dir_entry_cluster = cluster;
    dest->dir_entry_index = index;

    return FAT32_OK;
}

/**
 * @brief Helper to write to an entry with validation
 *
 * @param this inode which is related to the entry
 * @param src Source to write to
 * @return FAT32_STATUS 0 if success, 1 if failure
 */
FAT32_STATUS fat32_write_entry(VFS_INODE *this, FAT32_ENTRY *src) {
    if (!this || !src) return FAT32_ERR;

    FAT32_IDENT *id = (FAT32_IDENT *)(this->ident);
    if (!id) return FAT32_ERR;

    uint32_t cluster = src->dir_entry_cluster;
    size_t index = src->dir_entry_index;

    /* Validate cluster and index */
    if (!is_valid_cluster(cluster, id)) {
        return FAT32_ERR;
    }

    uint8_t dd[512] = {0};
    uint32_t sector = id->bs.cluster_begin_lba + (cluster - 2) * id->bs.sectors_per_cluster;
    sector += (index * sizeof(FAT_DIR_ENTRY)) / id->bs.bytes_per_sector;

    if (cached_sector_read(id->device, sector, dd) != 0) {
        return FAT32_ERR;
    }

    size_t entry_offset = (index * sizeof(FAT_DIR_ENTRY)) % id->bs.bytes_per_sector;
    FAT_DIR_ENTRY *de = (FAT_DIR_ENTRY *)(dd + entry_offset);

    memcpy(de->name, src->name, 11);
    de->attributes = src->attribute;
    de->first_cluster_high = (src->cluster_begin >> 16) & 0x0000FFFF;
    de->first_cluster_low = src->cluster_begin & 0x0000FFFF;
    de->size = src->file_size_bytes;

    char fn[VFS_MAX_NAME_LEN] = {0};
    fat32_get_short_filename(de->name, fn);
    klogd("FAT32 WRITE ENTRY: Modify directory entry \"%s\" to length %u\n",
          fn, src->file_size_bytes);

    return (cached_sector_write(id->device, sector, dd) == 0) ? FAT32_OK : FAT32_ERR;
}

/**
 * @brief Reads from an inode with improved cluster chain handling
 *
 * @param this inode to read from
 * @param offset Offset to start reading from
 * @param len Number of bytes to read
 * @param buff Buffer to copy into
 * @return int64_t Number of bytes read, -1 if error
 */
int64_t fat32_read(VFS_INODE *this, size_t offset, size_t len, void *buff) {
    if (!this || !buff || len == 0) {
        return -1;
    }

    FAT32_IDENT *id = (FAT32_IDENT *)(this->ident);
    if (!id) return -1;

    uint32_t cluster = id->entry.cluster_begin;
    if (!is_valid_cluster(cluster, id)) {
        return -1;
    }

    /* Calculate cluster and byte offset within cluster */
    uint32_t cluster_size = id->bs.sectors_per_cluster * id->bs.bytes_per_sector;
    uint32_t cluster_offset = offset / cluster_size;
    uint32_t byte_offset = offset % cluster_size;

    /* Navigate to starting cluster */
    uint32_t current_cluster = cluster;
    for (uint32_t i = 0; i < cluster_offset && current_cluster; i++) {
        current_cluster = fat32_get_next_cluster(current_cluster, id->FAT, id->fat_len);
        if (!is_valid_cluster(current_cluster, id) && current_cluster < 0x0FFFFFF8) {
            return -1;
        }
    }

    if (!current_cluster || current_cluster >= 0x0FFFFFF8) {
        return 0; /* End of file */
    }

    size_t bytes_read = 0;
    uint8_t *buffer = (uint8_t *)buff;

    while (bytes_read < len && current_cluster && current_cluster < 0x0FFFFFF8) {
        uint8_t cluster_data[cluster_size];

        /* Read entire cluster */
        for (uint32_t sector = 0; sector < id->bs.sectors_per_cluster; sector++) {
            uint32_t sector_lba = id->bs.cluster_begin_lba +
                                 (current_cluster - 2) * id->bs.sectors_per_cluster + sector;
            if (cached_sector_read(id->device, sector_lba,
                                 cluster_data + sector * id->bs.bytes_per_sector) != 0) {
                int64_t ret = (int64_t) bytes_read;
                return ret > 0 ? ret : -1;
            }
        }

        /* Copy relevant portion */
        size_t copy_start = (bytes_read == 0) ? byte_offset : 0;
        size_t copy_len = MIN(cluster_size - copy_start, len - bytes_read);

        memcpy(buffer + bytes_read, cluster_data + copy_start, copy_len);
        bytes_read += copy_len;

        /* Move to next cluster */
        current_cluster = fat32_get_next_cluster(current_cluster, id->FAT, id->fat_len);
    }

    return bytes_read;
}

/**
 * @brief Writes to an inode with improved cluster allocation
 *
 * @param this inode to write to
 * @param offset Offset in the file to start writing
 * @param len Number of bytes to write
 * @param buff Buffer to read from in order to write
 * @return int64_t number of bytes written, -1 if error
 */
int64_t fat32_write(VFS_INODE *this, size_t offset, size_t len, const void *buff) {
    if (!this || !buff || len == 0) {
        return -1;
    }

    FAT32_IDENT *id = (FAT32_IDENT *)(this->ident);
    if (!id) return -1;

    uint32_t cluster = id->entry.cluster_begin;
    uint32_t cluster_size = id->bs.sectors_per_cluster * id->bs.bytes_per_sector;

    /* If file is empty, allocate first cluster */
    if (!is_valid_cluster(cluster, id)) {
        cluster = fat32_get_free_cluster(id->FAT, id->fat_len);
        if (!cluster) {
            kloge("FAT32 WRITE: No free clusters available!\n");
            return -1;
        }
        id->FAT[cluster] = 0x0FFFFFFF;
        id->entry.cluster_begin = cluster;

        /* Write updated FAT to disk */
        size_t fat_sector = (cluster * 4) / id->bs.bytes_per_sector;
        cached_sector_write(id->device, id->bs.fat_begin_lba + fat_sector,
                           (uint8_t *)id->FAT + fat_sector * id->bs.bytes_per_sector);
    }

    /* Calculate starting position */
    uint32_t cluster_offset = offset / cluster_size;
    uint32_t byte_offset = offset % cluster_size;

    /* Navigate to starting cluster, allocating if necessary */
    uint32_t current_cluster = cluster;
    for (uint32_t i = 0; i < cluster_offset; i++) {
        uint32_t next_cluster = fat32_get_next_cluster(current_cluster, id->FAT, id->fat_len);
        if (!is_valid_cluster(next_cluster, id)) {
            /* Allocate new cluster */
            next_cluster = fat32_get_free_cluster(id->FAT, id->fat_len);
            if (!next_cluster) {
                kloge("FAT32 WRITE: No free clusters available!\n");
                return -1;
            }

            /* Update cluster chain */
            id->FAT[current_cluster] = next_cluster;
            id->FAT[next_cluster] = 0x0FFFFFFF;

            /* Write updated FAT sectors */
            size_t fat_sector1 = (current_cluster * 4) / id->bs.bytes_per_sector;
            size_t fat_sector2 = (next_cluster * 4) / id->bs.bytes_per_sector;

            cached_sector_write(id->device, id->bs.fat_begin_lba + fat_sector1,
                               (uint8_t *)id->FAT + fat_sector1 * id->bs.bytes_per_sector);

            if (fat_sector2 != fat_sector1) {
                cached_sector_write(id->device, id->bs.fat_begin_lba + fat_sector2,
                                   (uint8_t *)id->FAT + fat_sector2 * id->bs.bytes_per_sector);
            }
        }
        current_cluster = next_cluster;
    }

    size_t bytes_written = 0;
    const uint8_t *buffer = (const uint8_t *)buff;

    while (bytes_written < len && current_cluster && current_cluster < 0x0FFFFFF8) {
        uint8_t cluster_data[cluster_size];

        /* Read existing cluster data for partial writes */
        for (uint32_t sector = 0; sector < id->bs.sectors_per_cluster; sector++) {
            uint32_t sector_lba = id->bs.cluster_begin_lba +
                                 (current_cluster - 2) * id->bs.sectors_per_cluster + sector;
            cached_sector_read(id->device, sector_lba,
                              cluster_data + sector * id->bs.bytes_per_sector);
        }

        /* Modify cluster data */
        size_t write_start = (bytes_written == 0) ? byte_offset : 0;
        size_t write_len = MIN(cluster_size - write_start, len - bytes_written);

        memcpy(cluster_data + write_start, buffer + bytes_written, write_len);
        bytes_written += write_len;

        /* Write modified cluster back */
        for (uint32_t sector = 0; sector < id->bs.sectors_per_cluster; sector++) {
            uint32_t sector_lba = id->bs.cluster_begin_lba +
                                 (current_cluster - 2) * id->bs.sectors_per_cluster + sector;
            if (cached_sector_write(id->device, sector_lba,
                                   cluster_data + sector * id->bs.bytes_per_sector) != 0) {
                int64_t ret = (int64_t) bytes_written;
                return ret > 0 ? ret : -1;
            }
        }

        /* Move to next cluster or allocate if needed */
        if (bytes_written < len) {
            uint32_t next_cluster = fat32_get_next_cluster(current_cluster, id->FAT, id->fat_len);
            if (!is_valid_cluster(next_cluster, id)) {
                next_cluster = fat32_get_free_cluster(id->FAT, id->fat_len);
                if (!next_cluster) {
                    break; /* No more space */
                }

                id->FAT[current_cluster] = next_cluster;
                id->FAT[next_cluster] = 0x0FFFFFFF;

                /* Update FAT on disk */
                size_t fat_sector1 = (current_cluster * 4) / id->bs.bytes_per_sector;
                size_t fat_sector2 = (next_cluster * 4) / id->bs.bytes_per_sector;

                cached_sector_write(id->device, id->bs.fat_begin_lba + fat_sector1,
                                   (uint8_t *)id->FAT + fat_sector1 * id->bs.bytes_per_sector);

                if (fat_sector2 != fat_sector1) {
                    cached_sector_write(id->device, id->bs.fat_begin_lba + fat_sector2,
                                       (uint8_t *)id->FAT + fat_sector2 * id->bs.bytes_per_sector);
                }
            }
            current_cluster = next_cluster;
        }
    }

    /* Update file size if necessary */
    if (offset + bytes_written > id->entry.file_size_bytes) {
        id->entry.file_size_bytes = offset + bytes_written;
        fat32_write_entry(this, &(id->entry));
    }

    return bytes_written;
}

/**
 * @brief Remove a FAT32 node
 *
 * @param this VFS_TNODE to remove
 * @return FAT32_STATUS 0 if success, -1 if failure
 */
FAT32_STATUS fat32_rmnode(VFS_TNODE *this) {
    if (!this || !this->inode) return FAT32_ERR;
    FAT32_IDENT *id = (FAT32_IDENT *)(this->inode->ident);
    if (!id) return FAT32_ERR;

    /* Mark directory entry as deleted */
    FAT32_ENTRY *entry = &id->entry;
    entry->name[0] = 0xE5; /* Deleted marker */

    if (fat32_write_entry(this->inode, entry) != FAT32_OK) {
        return FAT32_ERR;
    }

    /* Free cluster chain */
    uint32_t cluster = entry->cluster_begin;
    while (is_valid_cluster(cluster, id)) {
        uint32_t next_cluster = fat32_get_next_cluster(cluster, id->FAT, id->fat_len);
        id->FAT[cluster] = 0; /* Mark as free */

        /* Update FAT on disk */
        size_t fat_sector = (cluster * 4) / id->bs.bytes_per_sector;
        cached_sector_write(id->device, id->bs.fat_begin_lba + fat_sector,
                           (uint8_t *)id->FAT + fat_sector * id->bs.bytes_per_sector);
        cluster = next_cluster;
    }

    return FAT32_OK;
}

/**
 * @brief Sync cached data to disk
 *
 * @param this inode
 * @return FAT32_STATUS 0 if success, -1 if failure
 */
FAT32_STATUS fat32_sync(VFS_INODE *this) {
    /* Flush all dirty cache entries */
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (sector_cache[i].dirty && sector_cache[i].data[0] != 0) {
            FAT32_IDENT *id = (FAT32_IDENT *)(this->ident);
            if (id && id->device) {
                ata_pio_write28(id->device, sector_cache[i].sector, 1,
                               sector_cache[i].data);
                sector_cache[i].dirty = 0;
            }
        }
    }
    return FAT32_OK;
}

/**
 * @brief Gets a directory entry from FAT with improved bounds checking
 *
 * @param this inode entry which is related to the directory entry
 * @param pos Position
 * @param dirent Directory entry to copy into
 * @return FAT32_STATUS 0 if success, -1 if failure
 */
FAT32_STATUS fat32_getdent(VFS_INODE *this, size_t pos, VFS_DIR_ENTRY *dirent) {
    if (!this || !dirent) return FAT32_ERR;

    size_t num = 0;
    size_t vector_size = vector_len(&fat32.files);

    for (size_t i = 0; i < vector_size; i++) {
        FAT32_IDENT_ITEM *item = vector_at(&fat32.files, i);
        if (!item || (uint64_t)item->parent != (uint64_t)this) {
            continue;
        }
        if (num == pos) {
            strncpy(dirent->name, item->name, VFS_MAX_NAME_LEN - 1);
            dirent->name[VFS_MAX_NAME_LEN - 1] = '\0';
            memcpy(&dirent->time, &item->time, sizeof(STD_TIME));
            dirent->type = VFS_FILE;
            if (item->entry.attributes & FAT32_ATTR_DIRECTORY) {
                dirent->type = VFS_DIRECTORY;
            }
            return FAT32_OK;
        }
        num++;
    }
    return FAT32_ERR;
}

/**
 * @brief FAT32 specific refresh function with improved error handling
 *
 * @param this inode to refresh
 * @return FAT32_STATUS -1 if error, 0 if success
 */
FAT32_STATUS fat32_refresh(VFS_INODE *this) {
    if (!this) return FAT32_ERR;

    FAT32_IDENT *id = (FAT32_IDENT *)(this->ident);
    if (!id) return FAT32_ERR;

    uint64_t cluster_len = id->bs.sectors_per_cluster * id->bs.bytes_per_sector;
    uint8_t *temp_buffer = (uint8_t *)(kmalloc(cluster_len));
    if (!temp_buffer) {
        kloge("Failed to create temporary buffer when refreshing FAT32!\n");
        return FAT32_ERR;
    }

    uint32_t temp_cluster = id->entry.cluster_begin;
    if (temp_cluster < 2) {
        temp_cluster = 2;
    }

    /* Clear existing entries for this directory */
    for (size_t i = 0; i < vector_len(&fat32.files); ) {
        FAT32_IDENT_ITEM *item = vector_at(&fat32.files, i);
        if (item && (uint64_t)item->parent == (uint64_t)this) {
            kfree(item);
            vector_erase(&fat32.files, i);
        } else {
            i++;
        }
    }

    while (is_valid_cluster(temp_cluster, id)) {
        /* Read cluster */
        for (uint32_t sector = 0; sector < id->bs.sectors_per_cluster; sector++) {
            uint32_t sector_lba = id->bs.cluster_begin_lba +
                                 (temp_cluster - 2) * id->bs.sectors_per_cluster + sector;
            if (cached_sector_read(id->device, sector_lba,
                                  temp_buffer + sector * id->bs.bytes_per_sector) != 0) {
                kfree(temp_buffer);
                return FAT32_ERR;
            }
        }

        /* Process directory entries */
        size_t entries_per_cluster = cluster_len / sizeof(FAT_DIR_ENTRY);
        for (size_t i = 0; i < entries_per_cluster; i++) {
            FAT_DIR_ENTRY *fe = (FAT_DIR_ENTRY *)(temp_buffer + i * sizeof(FAT_DIR_ENTRY));

            if (fe->name[0] == 0x00) {
                /* End of directory */
                goto refresh_done;
            }

            if (fe->name[0] == (char)(0xE5) || fe->attributes == 0) {
                /* Deleted or invalid entry */
                continue;
            }

            if ((fe->attributes & 0x0F) == 0x0F) {
                /* Long filename entry - skip for now */
                continue;
            }

            /* Process regular entry */
            char fn[VFS_MAX_NAME_LEN] = {0};
            fat32_get_short_filename((char *)fe->name, fn);

            FAT32_IDENT_ITEM *item = (FAT32_IDENT_ITEM *)
                                     (kmalloc(sizeof(FAT32_IDENT_ITEM)));
            if (!item) {
                kloge("Failed to allocate memory for new FAT32_IDENT_ITEM!\n");
                kfree(temp_buffer);
                return FAT32_ERR;
            }

            memcpy(&item->entry, fe, sizeof(FAT_DIR_ENTRY));
            strncpy(item->name, fn, VFS_MAX_NAME_LEN - 1);
            item->name[VFS_MAX_NAME_LEN - 1] = '\0';
            fat32_get_datetime(fe, &item->time);
            item->parent = this;
            vector_append(&fat32.files, (void *)item);
        }

        temp_cluster = fat32_get_next_cluster(temp_cluster, id->FAT, id->fat_len);
    }

refresh_done:
    kfree(temp_buffer);
    return FAT32_OK;
}

/**
 * @brief Helper to create a new FAT32 node
 *
 * @param this tnode to modify
 * @return FAT32_STATUS 0 if success, -1 if failure
 */
FAT32_STATUS fat32_mknode(VFS_TNODE *this) {
    if (!this || !this->inode) return FAT32_ERR;

    this->inode->ident = create_ident();
    return this->inode->ident ? FAT32_OK : FAT32_ERR;
}

/**
 * @brief Compares an entry and a path with proper bounds checking
 *
 * @param ent Entry to compare
 * @param path Path to compare
 * @return int 0 if same, not 0 if different
 */
static int fat32_compare_entry_and_path(FAT32_ENTRY *ent, const char *path) {
    if (!ent || !path) return -1;

    const char *ext = strchr(path, '.');

    char name[12] = {0};
    memset(name, ' ', 11);

    if (ext) {
        size_t name_len = MIN(ext - path, 8);
        size_t ext_len = MIN(strlen(ext + 1), 3);

        memcpy(name, path, name_len);
        memcpy(name + 8, ext + 1, ext_len);
    } else {
        size_t name_len = MIN(strlen(path), 8);
        memcpy(name, path, name_len);
    }

    /* Convert to uppercase for comparison */
    for (size_t i = 0; i < 11; i++) {
        name[i] = toupper(name[i]);
    }

    return memcmp(ent->name, name, 11);
}

/**
 * @brief Enhanced path parsing with better error handling
 *
 * @param this inode to compare again
 * @param path path to parse
 * @return FAT32_ENTRY entry located at path
 */
static FAT32_ENTRY fat32_parse_path(VFS_INODE *this, const char *path) {
    FAT32_ENTRY empty_entry = {0};

    if (!this || !path) return empty_entry;

    FAT32_IDENT *id = (FAT32_IDENT *)(this->ident);
    if (!id) return empty_entry;

    FAT32_ENTRY root = {
        .name = {' '},
        .attribute = FAT32_ATTR_DIRECTORY,
        .cluster_begin = id->bs.root_dir_first_cluster,
        .file_size_bytes = 0,
        .dir_entry_cluster = 0,
        .dir_entry_index = 0
    };

    if (!strcmp(path, "/")) {
        return root;
    }

    /* Skip leading slash */
    if (path[0] == '/') path++;

    uint32_t current_cluster = id->bs.root_dir_first_cluster;
    FAT32_ENTRY current_entry = root;

    /* Tokenize path */
    char path_copy[VFS_MAX_PATH_LEN];
    strncpy(path_copy, path, VFS_MAX_PATH_LEN - 1);
    path_copy[VFS_MAX_PATH_LEN - 1] = '\0';

    char *token = strtok(path_copy, "/");
    while (token && is_valid_cluster(current_cluster, id)) {
        int found = 0;
        uint32_t entry_index = 0;

        /* Search current directory for token */
        while (1) {
            FAT32_ENTRY ent = {0};
            if (fat32_read_entry(this, current_cluster, entry_index, &ent) != FAT32_OK) {
                /* Try next cluster in chain */
                current_cluster = fat32_get_next_cluster(current_cluster, id->FAT, id->fat_len);
                if (!is_valid_cluster(current_cluster, id)) {
                    break;
                }
                entry_index = 0;
                continue;
            }

            /* Skip invalid entries */
            if ((ent.attribute & 0x0F) == 0x0F || ent.name[0] == 0xE5) {
                entry_index++;
                continue;
            }

            if (ent.name[0] == 0x05) {
                ent.name[0] = 0xE5;
            }

            /* Compare with token */
            if (fat32_compare_entry_and_path(&ent, token) == 0) {
                current_entry = ent;
                current_cluster = ent.cluster_begin;
                found = 1;
                break;
            }

            entry_index++;

            /* Check if we've exceeded entries per cluster */
            size_t entries_per_cluster = (id->bs.sectors_per_cluster *
                                         id->bs.bytes_per_sector) / sizeof(FAT_DIR_ENTRY);
            if (entry_index >= entries_per_cluster) {
                current_cluster = fat32_get_next_cluster(current_cluster, id->FAT, id->fat_len);
                if (!is_valid_cluster(current_cluster, id)) {
                    break;
                }
                entry_index = 0;
            }
        }

        if (!found) {
            return empty_entry;
        }

        token = strtok(NULL, "/");
    }

    return current_entry;
}

/**
 * @brief Main FAT32 open function with improved error handling
 *
 * @param this inode to open
 * @param path path to inode
 * @return VFS_TNODE* tnode which is related to that inode
 */
VFS_TNODE *fat32_open(VFS_INODE *this, const char *path) {
    if (!this || !path) return NULL;

    /* Parse path to find parent directory */
    char parent_path[VFS_MAX_PATH_LEN] = {0};
    const char *last_slash = strrchr(path, '/');

    if (last_slash && last_slash != path) {
        size_t parent_len = last_slash - path;
        strncpy(parent_path, path, MIN(parent_len, VFS_MAX_PATH_LEN - 1));
        parent_path[MIN(parent_len, VFS_MAX_PATH_LEN - 1)] = '\0';
    } else {
        strcpy(parent_path, "/");
    }

    /* Get parent node */
    VFS_TNODE *parent_node = vfs_path_to_node(parent_path, NO_CREATE, 0);
    if (!parent_node) {
        return NULL;
    }

    /* Parse the requested entry */
    FAT32_ENTRY fe = fat32_parse_path(this, path);

    if (fe.attribute != 0 && (is_valid_cluster(fe.cluster_begin, (FAT32_IDENT*)this->ident) ||
                              fe.cluster_begin == 0)) {

        /* Create corresponding VFS node */
        VFS_NODE_TYPE node_type = (fe.attribute & FAT32_ATTR_DIRECTORY) ?
                                  VFS_DIRECTORY : VFS_FILE;

        VFS_TNODE *tnode = vfs_path_to_node(path, CREATE, node_type);
        if (!tnode) {
            return NULL;
        }

        tnode->inode->fs = &fat32;
        tnode->inode->size = fe.file_size_bytes;

        FAT32_IDENT *id = create_ident();
        if (!id) {
            return NULL;
        }

        /* Copy parent's FAT32 identity and set specific entry */
        memcpy(id, parent_node->inode->ident, sizeof(FAT32_IDENT));
        memcpy(&id->entry, &fe, sizeof(FAT32_ENTRY));

        tnode->inode->ident = (void *)id;
        return tnode;
    }

    return NULL;
}

/**
 * @brief Enhanced FAT32 mounting function with better validation
 *
 * @param at Where to mount at
 * @return VFS_INODE* New inode created at mounting location
 */
VFS_INODE *fat32_mount(VFS_INODE *at) {
    VFS_INODE *ret = vfs_alloc_inode(VFS_MOUNT_POINT, 0777, 0, &fat32, NULL);
    if (!ret) {
        kloge("FAT32 MOUNT: Failed to allocate inode!\n");
        return NULL;
    }

    ret->ident = create_ident();
    if (!ret->ident) {
        kfree(ret);
        return NULL;
    }

    FAT32_IDENT *id = (FAT32_IDENT *)(ret->ident);

    /* Get the ATA device */
    ATA_DEVICE *dev = at ? at->ident : NULL;
    if (!dev) {
        kloge("FAT32 MOUNT: Cannot mount FAT32 partition without specific device!\n");
        kfree(id);
        kfree(ret);
        return NULL;
    }

    klogi("FAT32 MOUNT: Mounting device %s\n",
          at->mount_point ? at->mount_point->name : "[NULL]");
    id->device = dev;

    /* Initialize cache */
    memset(sector_cache, 0, sizeof(sector_cache));
    cache_access_counter = 0;

    /* Read MBR */
    MBR mbr;
    if (ata_pio_read28(dev, 0, 1, (uint8_t *)&mbr) != 0) {
        kloge("FAT32 MOUNT: Failed to read MBR!\n");
        goto mount_error;
    }

    if (mbr.signature[0] != 0x55 || mbr.signature[1] != 0xAA) {
        kloge("FAT32 MOUNT: Invalid MBR signature!\n");
        goto mount_error;
    }

    /* Find FAT32 partition */
    int partition_found = 0;
    for (int i = 0; i < 4; i++) {
        if (mbr.partitions[i].type == 0x0B ||
            mbr.partitions[i].type == 0x0C ||
            mbr.partitions[i].type == 0x1C) {

            uint8_t dd[512] = {0};
            if (ata_pio_read28(dev, mbr.partitions[i].lba_start, 1, dd) != 0) {
                kloge("FAT32 MOUNT: Failed to read boot sector!\n");
                continue;
            }

            FAT_BS *fat_boot = (FAT_BS *)dd;

            /* Validate FAT32 parameters */
            if (fat_boot->table_size_16 != 0 || fat_boot->total_sectors_16 != 0) {
                kloge("FAT32 MOUNT: Partition %d contains FAT16 parameters!\n", i);
                continue;
            }

            if (fat_boot->bytes_per_sector == 0 ||
                fat_boot->sectors_per_cluster == 0 ||
                fat_boot->extbs32.table_size_32 == 0) {
                kloge("FAT32 MOUNT: Invalid boot sector parameters!\n");
                continue;
            }

            char volume_name[12] = {0};
            memcpy(volume_name, fat_boot->extbs32.volume_label, 11);
            klogi("FAT32 MOUNT: partition %d: [%s] is a FAT32 partition\n", i, volume_name);

            /* Store boot sector information */
            id->bs.bytes_per_sector         = fat_boot->bytes_per_sector;
            id->bs.sectors_per_cluster      = fat_boot->sectors_per_cluster;
            id->bs.reserved_sector_count    = fat_boot->reserved_sector_count;
            id->bs.num_fats                 = fat_boot->table_count;
            id->bs.sectors_per_fat          = fat_boot->extbs32.table_size_32;
            id->bs.root_dir_first_cluster   = fat_boot->extbs32.root_cluster;
            id->bs.total_sectors            = fat_boot->total_sectors_32;

            /* Calculate important LBAs */
            id->bs.fat_begin_lba = mbr.partitions[i].lba_start + id->bs.reserved_sector_count;
            id->bs.cluster_begin_lba = id->bs.fat_begin_lba +
                                      id->bs.num_fats * id->bs.sectors_per_fat;

            klogi("FAT32 MOUNT: ----- Mounting -----\n"
                  "\tOEM Name: %.8s\n"
                  "\tBytes per sector: %u\n"
                  "\tSectors per cluster: %u\n"
                  "\tNumber of reserved sectors: %u\n"
                  "\tSectors per file allocation table: %u\n"
                  "\tRoot directory first cluster: 0x%08x\n"
                  "\tFAT begin LBA: %u\n"
                  "\tCluster begin LBA: %u\n",
                  fat_boot->oem_name, id->bs.bytes_per_sector,
                  id->bs.sectors_per_cluster, id->bs.reserved_sector_count,
                  id->bs.sectors_per_fat, id->bs.root_dir_first_cluster,
                  id->bs.fat_begin_lba, id->bs.cluster_begin_lba);

            /* Allocate and read FAT */
            id->fat_len = id->bs.sectors_per_fat * id->bs.bytes_per_sector;
            id->FAT = (uint32_t *)(kmalloc(id->fat_len));
            if (!id->FAT) {
                kloge("FAT32 MOUNT: Failed to allocate memory for FAT!\n");
                goto mount_error;
            }

            /* Read FAT from disk */
            if (ata_pio_read28(id->device, id->bs.fat_begin_lba,
                              id->bs.sectors_per_fat, (void *)id->FAT) != 0) {
                kloge("FAT32 MOUNT: Failed to read FAT from disk!\n");
                kfree(id->FAT);
                goto mount_error;
            }

            /* Validate FAT signature */
            if ((id->FAT[0] & 0x0FFFFFF8) != 0x0FFFFFF8 ||
                (id->FAT[1] & 0x0FFFFFFF) != 0x0FFFFFFF) {
                kloge("FAT32 MOUNT: Invalid FAT signature!\n");
                kfree(id->FAT);
                goto mount_error;
            }

            partition_found = 1;
            break;
        }
    }

    if (!partition_found) {
        kloge("FAT32 MOUNT: No valid FAT32 partition found!\n");
        goto mount_error;
    }

    klogi("FAT32 MOUNT: Mount partition finished successfully\n");
    return ret;

mount_error:
    if (id) {
        if (id->FAT) kfree(id->FAT);
        kfree(id);
    }
    if (ret) kfree(ret);
    return NULL;
}