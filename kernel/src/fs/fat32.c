/**
 * @file fat32.c
 * @author Zack Bostocj
 * @brief Functionality related to FAT32 filesystem
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <stdint.h>

#include <common/kmalloc.h>
#include <common/vector.h>
#include <common/math.h>
#include <common/kprint.h>

#include <sys/cpu.h>
#include <sys/asm.h>

#include <fs/vfs.h>
#include <fs/fat32.h>

#include <dev/storage/ata.h>

VFS_FS fat32 = {
    .name = "fat32",
    .is_temp = 0,
    .files = {0},
    .open = fat32_open,
    .mount = fat32_mount,
    .mknode = fat32_mknode,
    .rmnode = NULL,
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
 * @brief Helper to read an entry
 * 
 * @param this inode which is related to this entry
 * @param cluster Cluter number
 * @param index Index
 * @param dest Destination buffer to copy data to
 * @return FAT32_STATUS 0 if success, 1 if failure
 */
FAT32_STATUS fat32_read_entry(VFS_INODE* this, uint32_t cluster, size_t index,
                              FAT32_ENTRY *dest) {
    FAT32_IDENT *id = (FAT32_IDENT *)(this->ident);

    /* TODO: Need to add cache to speed up reading */
    uint8_t dd[512] = {0};
    ata_pio_read28(id->device, id->bs.cluster_begin_lba + (cluster - 2) *
                   id->bs.sectors_per_cluster, 1, dd);
    
    FAT_DIR_ENTRY *de = (FAT_DIR_ENTRY *)dd;

    memcpy(dest->name, de[index].name, 11);
    dest->attribute = de[index].attributes;
    dest->cluster_begin = (uint32_t)de[index].first_cluster_high << 16;
    dest->cluster_begin = de[index].first_cluster_low | dest->cluster_begin;
    dest->file_size_bytes = de[index].size;

    dest->dir_entry_cluster = cluster;
    dest->dir_entry_index = index;

    return FAT32_OK;
}

/**
 * @brief Helper to write to an entry
 *
 * @param this inode which is related to the entry 
 * @param src Source to write to
 * @return FAT32_STATUS 0 if success, 1 if failure
 */
FAT32_STATUS fat32_write_entry(VFS_INODE *this, FAT32_ENTRY *src) {
    FAT32_IDENT *id = (FAT32_IDENT *)(this->ident);
    uint32_t cluster = src->dir_entry_cluster;
    size_t index = src->dir_entry_index;

    /* TODO: Add cache to speed up reading */
    uint8_t dd[512] = {0};
    ata_pio_read28(id->device, id->bs.cluster_begin_lba + (cluster - 2) *
                   id->bs.sectors_per_cluster, 1, dd);

    FAT_DIR_ENTRY *de = (FAT_DIR_ENTRY *)dd;

    memcpy(de[index].name, src->name, 11);
    de[index].attributes = src->attribute;
    de[index].first_cluster_high = (src->cluster_begin >> 16) & 0x0000FFFF;
    de[index].first_cluster_low = src->cluster_begin & 0x0000FFFF;
    de[index].size = src->file_size_bytes;

    char fn[VFS_MAX_NAME_LEN] = {0};
    fat32_get_short_filename(de[index].name, fn);
    klogi("FAT32 WRITE ENTRY: Modify directory of entry %d to length %d\n",
           fn, src->file_size_bytes);
    
    ata_pio_write28(id->device, id->bs.cluster_begin_lba + (cluster - 2) *
                    id->bs.sectors_per_cluster, 1, dd);
    return FAT32_OK;
}

/**
 * @brief Reads from an inode
 * 
 * @param this inode to read from
 * @param offset Offset to start reading from
 * @param len Number of bytes to read
 * @param buff Buffer to copy into
 * @return int64_t Number of bytes read, -1 if error
 */
int64_t fat32_read(VFS_INODE *this, size_t offset, size_t len, void *buff) {
    /* TODO: Consider when multiple sectors are not continuous */
    if (!buff) {
        return -1;
    }
    FAT32_IDENT *id = (FAT32_IDENT *)(this->ident);
    uint32_t cluster = id->entry.cluster_begin;
    uint32_t sector_num = DIV_ROUNDUP(offset + len, id->bs.bytes_per_sector);
    uint8_t *dd = (uint8_t *)(kmalloc(sector_num * id->bs.bytes_per_sector));
    if (!dd) {
        kloge("Failed to allocate memory for a copy buffer when reading FAT32!\n");
        halt();
    }

    uint32_t temp_cluster = cluster;
    size_t temp_readlen = 0;

    while (1) {
        ata_pio_read28(id->device,
                       id->bs.cluster_begin_lba + (temp_cluster - 2) * id->bs.sectors_per_cluster,
                       id->bs.sectors_per_cluster,
                       &dd[temp_readlen]);
        temp_readlen += id->bs.bytes_per_sector * id->bs.sectors_per_cluster;
        temp_cluster = fat32_get_next_cluster(temp_cluster, id->FAT, id->fat_len);
        /* Make sure that we don't overread anything */
        if (!temp_cluster) {
            break;
        }
    }

    size_t retlen = MIN(sector_num * id->bs.bytes_per_sector - offset, len);
    memcpy(buff, &dd[offset], retlen);
    kfree(dd);

    return retlen;
}
/**
 * @brief Writes to an inode
 * 
 * @param this inode to write to
 * @param offset Offset in the file to start writing
 * @param len Number of bytes to write
 * @param buff Buffer to read from in order to write
 * @return int64_t number of bytes written, -1 if error
 */
int64_t fat32_write(VFS_INODE *this, size_t offset, size_t len, const void *buff) {
    if (!buff) {
        return -1;
    }
    FAT32_IDENT *id = (FAT32_IDENT *)(this->ident);
    uint32_t cluster = id->entry.cluster_begin;
    size_t sector_num = DIV_ROUNDUP(offset + len, id->bs.bytes_per_sector);
    uint8_t *dd = (uint8_t *)(kmalloc(sector_num * id->bs.bytes_per_sector));
    if (!dd) {
        kloge("Failed to allocate memory for a copy buffer when writing FAT32!\n");
        return -1;
    }

    fat32_read(this, 0, sector_num * id->bs.bytes_per_sector, dd);
    memcpy(dd + offset, buff, len);

    uint32_t temp_cluster = cluster;
    size_t temp_writelen = 0;

    while (1) {
        ata_pio_write28(id->device,
                        id->bs.cluster_begin_lba + (cluster - 2) * id->bs.sectors_per_cluster,
                        id->bs.sectors_per_cluster,
                        &dd[temp_writelen]);
        temp_writelen += id->bs.bytes_per_sector * id->bs.bytes_per_sector;

        /* Make sure that we don't overwrite anything */
        if (temp_writelen >= sector_num * id->bs.bytes_per_sector) {
            break;
        }

        temp_cluster = fat32_get_next_cluster(cluster, id->FAT, id->fat_len);
        if (!temp_cluster) {
            /* Allocate new cluster and update cluster chain in FAT */
            temp_cluster = fat32_get_free_cluster(id->FAT, id->fat_len);

            /* Update the FAT */
            id->FAT[cluster] = temp_cluster;
            id->FAT[temp_cluster] = 0x0FFFFFFF;

            size_t fat_sector_no = DIV_ROUNDUP(cluster * 4, id->bs.bytes_per_sector);

            ata_pio_write28(id->device,
                            id->bs.fat_begin_lba + fat_sector_no - 1,
                            1,
                            (uint8_t *)id->FAT + (fat_sector_no - 1) * id->bs.bytes_per_sector);
            
            size_t temp_fat_sector_no = DIV_ROUNDUP(temp_cluster * 4,
                                                    id->bs.bytes_per_sector);
            
            if (temp_fat_sector_no != fat_sector_no) {
            ata_pio_write28(id->device,
                            id->bs.fat_begin_lba + temp_fat_sector_no - 1,
                            1,
                            (uint8_t *)id->FAT + (temp_fat_sector_no - 1) * id->bs.bytes_per_sector);
            }
        }
        cluster = temp_cluster;
    }

    size_t retlen = MIN(sector_num * id->bs.bytes_per_sector - offset, len);
    kfree(dd);

    /* Update the file entry */
    if (offset + len > id->entry.file_size_bytes) {
        id->entry.file_size_bytes = offset + len;
        fat32_write_entry(this, &(id->entry));
    }

    return retlen;
}

/**
 * @brief Doesn't do anything in this context
 * 
 * @param this inode
 * @return FAT32_STATUS 0 if success, -1 if failure
 */
FAT32_STATUS fat32_sync(VFS_INODE *this) {
    (void) this;
    return 0;
}

/**
 * @brief Gets a directory entry from FAT
 * 
 * @param this inode entry which is related to the directory entry
 * @param pos Position
 * @param dirent Directory entry to copy into
 * @return FAT32_STATUS 0 if success, -1 if failure
 */
FAT32_STATUS fat32_getdent(VFS_INODE *this, size_t pos, VFS_DIR_ENTRY *dirent) {
    size_t num = 0;
    for (size_t i = 0; i < vector_len(&fat32.files); i++) {
        FAT32_IDENT_ITEM *item = vector_at(&fat32.files, i);
        if ((uint64_t)item->parent != (uint64_t)this) {
            continue;
        }
        if (num == pos) {
            strcpy(dirent->name, item->name);
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
 * @brief FAT32 specific refresh function
 * 
 * @param this inode to refresh 
 * @return FAT32_STATUS -1 if error, 0 if success
 */
FAT32_STATUS fat32_refresh(VFS_INODE *this) {
    FAT32_IDENT *id = (FAT32_IDENT *)(this->ident);

    uint64_t cluster_len = id->bs.sectors_per_cluster * id->bs.bytes_per_sector;
    uint64_t temp_len = cluster_len;
    uint8_t *temp_buffer = (uint8_t *)(kmalloc(temp_len));
    if (!temp_buffer) {
        kloge("Failed to create temporary buffer when refreshing FAT32!\n");
        halt();
    }

    uint32_t temp_cluster = id->entry.cluster_begin;

    if (temp_cluster < 2) {
        temp_cluster = 2;
    }

    while (TRUE) {
        ata_pio_read28(id->device,
                       id->bs.cluster_begin_lba + (temp_cluster - 2) * id->bs.sectors_per_cluster,
                       id->bs.sectors_per_cluster,
                       temp_buffer);
        temp_cluster = fat32_get_next_cluster(temp_cluster, id->FAT, id->fat_len);

        for (size_t i = 0; i < temp_len / sizeof(FAT_DIR_ENTRY);) {
            FAT_DIR_ENTRY *fe;
            FAT_DIR_ENTRY *fe1;

            fe = (FAT_DIR_ENTRY *)(temp_buffer + i * sizeof(FAT_DIR_ENTRY));
            if (fe->attributes == 0) {
                i++;
                continue;
            }

            fe1 = (FAT_DIR_ENTRY *)(temp_buffer + temp_len - sizeof(FAT_DIR_ENTRY));
            char fn[VFS_MAX_NAME_LEN] = {0};
            int lfn_meet = 0;
            uint8_t lfn_checksum = 0;

            /* Process the long file name */
            if (fe->attributes & FAT32_ATTR_LONGNAME) {
                lfn_meet = 1;
                lfn_checksum = ((FAT_LFN_ENTRY *) fe)->dos_checksum;
                size_t count = fat32_get_long_name((FAT_LFN_ENTRY *)fe,
                                                   (FAT_LFN_ENTRY *)fe1,
                                                   fn);
                if (count > 0) {
                    i += count;
                } else {
                    if (temp_cluster == 0) {
                        break;
                    } else if (temp_cluster >= 0xFFFFFFF8) {
                        break;
                    }

                    /* Need to load more data from disk */
                    /* TODO: more testing needs to be done on this */
                    uint8_t *buf = (uint8_t *)(kmalloc(temp_len + cluster_len - i * sizeof(FAT_DIR_ENTRY)));
                    memcpy(buf, &temp_buffer[i * sizeof(FAT_DIR_ENTRY)],
                           temp_len - i * sizeof(FAT_DIR_ENTRY));
                    ata_pio_read28(id->device,
                                   id->bs.cluster_begin_lba + (temp_cluster - 2) * id->bs.sectors_per_cluster,
                                   id->bs.sectors_per_cluster,
                                   &buf[i * sizeof(FAT_DIR_ENTRY)]);
                    kfree(temp_buffer);
                    temp_buffer = buf;
                    temp_len += cluster_len - i * sizeof(FAT_DIR_ENTRY);
                    /* Restart the processing */
                    i = 0;
                    continue;
                }
            }

            /* Short file name must be consistent with long name */
            fe = (FAT_DIR_ENTRY *)(temp_buffer + i * sizeof(FAT_DIR_ENTRY));
            if (strlen(fn) == 0) {
                fat32_get_short_filename((char *)fe->name, fn);
            }

            if (!(lfn_meet && lfn_checksum == fat32_checksum((char *)fe->name))) {
                klogi("FAT32 REFRESH: file attribute %d, name \"%s\"\n", fe->attributes, fn);
            }

            FAT32_IDENT_ITEM *item = (FAT32_IDENT_ITEM *)(kmalloc(sizeof(FAT32_IDENT_ITEM)));
            if (item) {
                memcpy(&item->entry, fe, sizeof(FAT_DIR_ENTRY));
                strcpy(item->name, fn);
                fat32_get_datetime(fe, &item->time);
                item->parent = this;
                vector_append(&fat32.files, (void *)item);
            } else {
                kloge("Failed to allocate memory for new FAT32_IDENT_ITEM!\n");
                halt();
            }
            i++;
        }

        if (!temp_cluster) {
            break;
        } else if (temp_cluster >= 0xFFFFFFF8) {
            break;
        }
    }

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
    this->inode->ident = create_ident();
    return FAT32_OK;
}

/**
 * @brief Compares an entry and a path
 * 
 * @param ent Entry to compare
 * @param path Path to compare
 * @return int 0 if same, not 0 if different
 */
static int fat32_compare_entry_and_path(FAT32_ENTRY *ent, const char *path) {
    const char *ext = strchr(path, '.');

    char name[12] = {0};
    memset(name, ' ', 11);
    if (ext) {
        memcpy(name, path, MIN(ext - path - 1, 8));
        memcpy(name + 8, ext, MIN(strlen(ext), 3));
    } else {
        memcpy(name, path, MIN(strlen(path), 8));
    }

    for (size_t i = 0; i < 11; i++) {
        name[i] = toupper(name[i]);
    }

    return memcmp(ent->name, name, 11);
}

/**
 * @brief Parses the path given
 * 
 * @param this inode to compare again
 * @param path path to parse
 * @return FAT32_ENTRY entry located at path
 */
static FAT32_ENTRY fat32_parse_path(VFS_INODE *this, const char *path) {
    FAT32_IDENT *id = (FAT32_IDENT *)(this->ident);
    FAT32_ENTRY root = {
        .name = {' '},
        .attribute = FAT32_ATTR_DIRECTORY,
        .cluster_begin = id->bs.root_dir_first_cluster,
        .file_size_bytes = 0,
        .dir_entry_cluster = 0, /* TODO: Reconsider this */
        .dir_entry_index = 0
    };

    if (!strcmp(path, "/")) {
        return root;
    }

    uint32_t cluster = id->bs.root_dir_first_cluster;
    uint32_t i = 0;
    path++;
    FAT32_ENTRY ent = {0};
    for (;; i++) {
        if (!fat32_read_entry(this, cluster, i, &ent)) {
            break;
        } else if (i == id->bs.sectors_per_cluster * 16) {
            i = 0;
            cluster = fat32_get_next_cluster(cluster, id->FAT, id->fat_len);
            if ((cluster * id->bs.sectors_per_cluster * 16) >= id->bs.total_sectors) {
                break;
            }
        }

        if ((ent.attribute & 0x0F) == 0x0F) {
            /* Skip long file name */
            continue;
        } else if (ent.name[0] == 0xE5) {
            continue;
        } else if (ent.name[0] == 0x05) {
            ent.name[0] = 0xE5;
        }
    }

    char sub_elem[VFS_MAX_PATH_LEN] = {0};
    int top_level = 0;

    const char *slash = strchr(path, '/');
    if (!slash) {
        memcpy(sub_elem, path, strlen(path));
        top_level = 1;
    } else {
        memcpy(sub_elem, path, slash - path - 1);
        sub_elem[slash - path - 1] = '\0';
        path += strlen(sub_elem) + 1;
    }

    if (fat32_compare_entry_and_path(&ent, sub_elem)) {
        if (top_level) {
            /* Found file */
            return ent;
        } else {
            /* Found directory entry matching the sub_elem */
            cluster = ent.cluster_begin;
            i = 0;
        }
    }

    FAT32_ENTRY err = {0};
    return err;
}

/**
 * @brief Main FAT32 open function
 * 
 * @param this inode to open
 * @param path path to inode
 * @return VFS_TNODE* tnode which is related to that inode
 */
VFS_TNODE *fat32_open(VFS_INODE *this, const char *path) {
    size_t last_idx = 0;
    size_t cur_idx = 0;
    size_t dir_count = 0;

    for (cur_idx = 0; cur_idx < strlen(path); cur_idx++) {
        if (path[cur_idx] == '/') {
            if (cur_idx - last_idx > 1) {
                dir_count++;
            }
            if (dir_count == 2) {
                break;
            }
            last_idx = cur_idx;
        }
    }

    char rootpath[VFS_MAX_PATH_LEN];
    strcpy(rootpath, path);
    rootpath[cur_idx] = '\0';
    VFS_TNODE *root_node = vfs_path_to_node(rootpath, NO_CREATE, 0);

    FAT32_ENTRY fe = fat32_parse_path(this, &path[cur_idx]);

    if (fe.attribute != 0 && fe.cluster_begin != 0) {
        /* Create parent directory */
        for (size_t i = cur_idx + 1; i < strlen(path); i++) {
            if (path[i] == '/') {
                static char tempbuff[VFS_MAX_NAME_LEN];
                strcpy(tempbuff, path);
                tempbuff[i] = '\0';
                /* Calling here will not increment reference count */
                fat32_open(this, tempbuff);
            }
        }

        /* Create corresponding node */
        if (fe.attribute & FAT32_ATTR_DIRECTORY) {
            vfs_path_to_node(path, CREATE, VFS_DIRECTORY);
        } else {
            vfs_path_to_node(path, CREATE, VFS_FILE);
        }

        VFS_TNODE *tnode = vfs_path_to_node(path, NO_CREATE, 0);

        tnode->inode->fs = &fat32;
        tnode->inode->size = fe.file_size_bytes;

        FAT32_IDENT *id = create_ident();
        memcpy(id, root_node->inode->ident, sizeof(FAT32_IDENT));
        memcpy(&id->entry, &fe, sizeof(FAT32_ENTRY));

        tnode->inode->ident = (void *)(id);

        return tnode;
    } else {
        return NULL;
    }
}

/**
 * @brief Main FAT32 mounting function
 *
 * @param at Where to mount at
 * @return VFS_INODE* New inode created at mounting location
 */
VFS_INODE *fat32_mount(VFS_INODE *at) {
    VFS_INODE *ret = vfs_alloc_inode(VFS_MOUNT_POINT, 0777, 0, &fat32, NULL);
    ret->ident = create_ident();
    FAT32_IDENT *id = (FAT32_IDENT *)(ret->ident);

    /* Get the ATA device */
    ATA_DEVICE *dev = at ? at->ident : NULL;
    if (!dev) {
        kloge("FAT32 MOUNT: Cannot mount FAT32 partition without specific device!\n");
    } else {
        klogi("FAT32 MOUNT: Mounting device %s\n", at->mount_point ? at->mount_point->name : "[NULL]");
    }
    id->device = dev;

    /* TODO: Consider the scenario when the disk has more than one FAT */
    MBR mbr;
    ata_pio_read28(dev, 0, 1, (uint8_t *)&mbr);

    if (mbr.signature[0] == 0x55 && mbr.signature[1] == 0xAA) {
        for (int i = 0; i < 4; i++) {
            /* It is FAT32 partition */
            if (mbr.partitions[i].type == 0x0B ||
                mbr.partitions[i].type == 0x0C ||
                mbr.partitions[i].type == 0x1C) {
                uint8_t dd[512] = {0};

                /* Read and copy into buffer */
                ata_pio_read28(dev, mbr.partitions[i].lba_start, 1, dd);

                /* This should be the bootsector */
                FAT_BS *fat_boot = (FAT_BS *)dd;
                if (fat_boot->table_size_16 != 0 || fat_boot->total_sectors_16 != 0) {
                    kloge("FAT32 filesystem contains FAT16 parameters!\n");
                    halt();
                }

                char volume_name[12] = {0};
                memcpy(volume_name, fat_boot->extbs32.volume_label, 11);
                klogi("FAT32 MOUNT: parition %d: [%s] is a FAT32 partition of device %x\n",
                      i, volume_name, dev);

                id->bs.bytes_per_sector         = fat_boot->bytes_per_sector;
                id->bs.sectors_per_cluster      = fat_boot->sectors_per_cluster;
                id->bs.reserved_sector_count    = fat_boot->reserved_sector_count;
                id->bs.num_fats                 = fat_boot->table_count;
                id->bs.sectors_per_fat          = fat_boot->extbs32.table_size_32;
                id->bs.root_dir_first_cluster   = fat_boot->extbs32.root_cluster;
                id->bs.total_sectors            = fat_boot->total_sectors_32;

                klogi("FAT32 MOUNT: ----- Mounting -----\n"
                      "\tOEM Name: %s\n"
                      "\tBytes per sector: %d\n"
                      "\tSectors per cluster: %d\n"
                      "\tNumber of reserved sectors: %d\n"
                      "\tSectors per file allocation table: %d\n"
                      "\tRoot directory first cluster %2x\n",
                      i, fat_boot->oem_name, id->bs.bytes_per_sector,
                      id->bs.sectors_per_cluster, id->bs.reserved_sector_count,
                      id->bs.num_fats,
                      id->bs.sectors_per_fat, id->bs.root_dir_first_cluster);

                /* FAT 1 sector number */
                id->bs.fat_begin_lba = mbr.partitions[i].lba_start +
                                       id->bs.reserved_sector_count;
                /* Cluster sector number */
                id->bs.cluster_begin_lba = id->bs.fat_begin_lba +
                                           id->bs.num_fats *
                                           id->bs.sectors_per_fat;

                id->fat_len = id->bs.sectors_per_fat * id->bs.bytes_per_sector;
                id->FAT = (uint32_t *)(kmalloc(id->fat_len));
                if (!id->FAT) {
                    kloge("FAT32 MOUNT: Failed to allocate memory for new FAT!\n");
                    halt();
                }

                ata_pio_read28(id->device, id->bs.fat_begin_lba,
                               id->bs.sectors_per_fat, (void *)id->FAT);
                break;
            }
        }
    }

    klogi("FAT32 MOUNT: Mount partition finished\n");
    return ret;
}