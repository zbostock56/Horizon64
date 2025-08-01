/**
 * @file fat32.h
 * @author Zack Bostock
 * @brief Information pertaining to FAT32 filesystem
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <common/time.h>

#include <structs/fat32_str.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
typedef int64_t FAT32_STATUS;
#define FAT32_OK        (0)
#define FAT32_ERR       (-1)

#define FAT32_ATTR_READ_ONLY        (0x01)
#define FAT32_ATTR_HIDDEN           (0x02)
#define FAT32_ATTR_SYSTEM           (0x04)
#define FAT32_ATTR_VOLUME_ID        (0x08)
#define FAT32_ATTR_DIRECTORY        (0x10)
#define FAT32_ATTR_ARCHIVE          (0x20)
#define FAT32_ATTR_LONGNAME         (0x0F)

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */
#define SECTOR_SHIFT                (9)
#define SECTOR_TO_OFFSET(x)         ((x) << SECTOR_SHIFT)
#define CLUSTER_TO_OFFSET(x, c, s)  (SECTOR_SHIFT((c) + ((x) - 2) * (s)))

/* --------------------------- INTERNALLY DEFINED --------------------------- */
FAT32_STATUS fat32_read_entry(VFS_INODE* this, uint32_t cluster, size_t index,
                              FAT32_ENTRY *dest);
FAT32_STATUS fat32_write_entry(VFS_INODE *this, FAT32_ENTRY *src);
int64_t fat32_read(VFS_INODE *this, size_t offset, size_t len, void *buff);
int64_t fat32_write(VFS_INODE *this, size_t offset, size_t len, const void *buff);
FAT32_STATUS fat32_sync(VFS_INODE *this);
FAT32_STATUS fat32_getdent(VFS_INODE *this, size_t pos, VFS_DIR_ENTRY *dirent);
FAT32_STATUS fat32_refresh(VFS_INODE *this);
FAT32_STATUS fat32_mknode(VFS_TNODE *this);
FAT32_STATUS fat32_rmnode(VFS_TNODE *this);
VFS_TNODE *fat32_open(VFS_INODE *this, const char *path);
VFS_INODE *fat32_mount(VFS_INODE *at);

/**
 * @brief Helper to find the first free cluster in the file allocation table
 * 
 * @param fat File allocation table
 * @param fat_len File allocation table length
 * @return uint32_t First free cluster in file allocation table
 */
static inline uint32_t fat32_get_free_cluster(uint32_t *fat, size_t fat_len) {
    if (fat) {
        for (size_t i = 0; i < fat_len; i++) {
            if (!fat[i]) {
                return i;
            }
        }
    }
    return 0;
}

/**
 * @brief Helper to get the next cluster
 * 
 * @param cluster Current cluster
 * @param fat File allocation table
 * @param fat_len File allocation table length
 * @return uint32_t Next cluster
 */
static inline uint32_t fat32_get_next_cluster(uint32_t cluster, uint32_t *fat, size_t fat_len) {
    if (!fat) {
        return 0;
    }
    if (cluster >= (fat_len / 4)) {
        return 0;
    } else if (fat[cluster] >= 0xFFFFFFF8) {
        return 0;
    } else if (fat[cluster] == 0x0FFFFFFF) {
        return 0;
    }
    return fat[cluster];
}

/**
 * @brief Helper to get the short filename given a file name and extension
 * 
 * @param file_name File name + extension
 * @param dest Buffer to copy into
 */
static inline void fat32_get_short_filename(char *file_name, char *dest) {
    if (!dest || !file_name) {
        return;
    }

    size_t i = 0;
    char fn[9] = {0};
    char ext[4] = {0};

    /* Copy over into buffers which can be modified */
    memcpy(fn, file_name, 8);
    memcpy(ext, &(file_name[8]), 3);

    while (1) {
        if (fn[i] == 0x20 || fn[i] == '\0') {
            fn[i] = '\0';
            break;
        }
        i++;
    }

    i = 0;
    while (1) {
    if (ext[i] == 0x20 || ext[i] == '\0') {
            ext[i] = '\0';
            break;
        }
        i++;
    }

    strcpy(dest, fn);
    if (strlen(ext) > 0) {
        strcat(dest, ".");
        strcat(dest, ext);
    }
}


/**
 * @brief Similar to strcpy and strncpy, but doesn't stop on \0 bytes
 * 
 * @param dest Buffer to copy into
 * @param src Source to copy from
 * @param len Number of bytes to copy
 */
static inline void fat32_name_copy(char *dest, char *src, size_t len) {
    while (len--) {
        *dest++ = *src++;
        src++;
    }
}

/**
 * @brief Helper to get long name
 * 
 * @param lfne First lfne
 * @param lfne_last Last lfne
 * @param dest Buffer to copy into
 * @return uint32_t Number of butes copied
 */
static inline uint32_t fat32_get_long_name(FAT_LFN_ENTRY *lfne, FAT_LFN_ENTRY *lfne_last, char *dest) {
    char fn_temp[VFS_MAX_NAME_LEN] = {0};
    uint32_t count = 0;
    uint32_t entry_num = 0;

    if (!dest) {
        return 0;
    }

    dest[0] = '\0';
    while (1) {
        strcpy(fn_temp, dest);
        count++;

        uint32_t i = lfne->sequence_number & 0x3F;
        int first = lfne->sequence_number & 0x40;
        fat32_name_copy(dest, lfne->name1, 5);
        fat32_name_copy(&dest[5], lfne->name2, 6);
        fat32_name_copy(&dest[11], lfne->name3, 2);
        dest[13] = '\0';

        strcat(dest, fn_temp);

        if (first) {
            entry_num = i;
        }
        if (i == 1 && count == entry_num) {
            return count;
        }

        if (((uint64_t)lfne) == ((uint64_t)lfne_last)) {
            break;
        }
        lfne = (FAT_LFN_ENTRY *)((uint64_t)lfne + sizeof(FAT_LFN_ENTRY));
    }

    return 0;
}

/**
 * @brief Helper to checksum a filename
 * 
 * @param name Name to checksum
 * @return uint8_t Checksum
 */
static inline uint8_t fat32_checksum(const char *name) {
    if (!name) {
        return 0;
    }

    uint8_t s = name[0];
    s = (s << 7) + (s >> 1) + name[1];
    s = (s << 7) + (s >> 1) + name[2];
    s = (s << 7) + (s >> 1) + name[3];
    s = (s << 7) + (s >> 1) + name[4];
    s = (s << 7) + (s >> 1) + name[5];
    s = (s << 7) + (s >> 1) + name[6];
    s = (s << 7) + (s >> 1) + name[7];
    s = (s << 7) + (s >> 1) + name[8];
    s = (s << 7) + (s >> 1) + name[9];
    s = (s << 7) + (s >> 1) + name[10];
    return s;
}

/**
 * @brief Helper to get the date and time of FAT32 directory entry and convert
 * to STD_TIME
 * 
 * @param de Directory entry to get time of
 * @param t Resulting STD_TIME of the directory entry
 */
static inline void fat32_get_datetime(FAT_DIR_ENTRY *de, STD_TIME *t) {
    int year = ((de->modified_date & 0xFE00) >> 9) + 1980;
    int month = (de->modified_date & 0x01E0) >> 5;
    int day = de->modified_date & 0x001F;
    int hour = ((de->modified_time & 0xF800) >> 11);
    int min = (de->modified_time & 0x07E0) >> 5;
    int sec = (de->modified_time & 0x001F) * 2;

    const uint64_t time = sec_in_years(year - 1) +
                          sec_in_months(month - 1, year) +
                          ((day - 1) * 86400) +
                          (hour * 3600) +
                          (min * 60) +
                          sec;

    seconds_to_std_time(time, t);
}