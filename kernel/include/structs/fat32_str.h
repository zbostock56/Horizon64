/**
 * @file fat32_str.h
 * @author Zack Bostock
 * @brief Structures relate to the FAT32 filesystem
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <stdint.h>

#include <common/lock.h>
#include <common/time.h>

#include <structs/vfs_str.h>
#include <structs/ata_str.h>

/**
 * @brief Extended boot sector for FAT32
 */
typedef struct {
    uint32_t table_size_32;
    uint16_t extended_flags;
    uint16_t fat_version;
    uint32_t root_cluster;
    uint16_t fat_info;
    uint16_t backup_bs_sector;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t reserved_1;
    uint8_t boot_signature;
    uint32_t volume_id;
    uint8_t volume_label[11];
    uint8_t fat_type_lable[8];
} __attribute__((packed)) FAT_EXTBS_32;

/**
 * @brief Extended boot sector for FAT16
 */
typedef struct {
    uint8_t drive_number;
    uint8_t reserved_1;
    uint8_t boot_signature;
    uint32_t volume_id;
    uint8_t volume_label[11];
    uint8_t fat_type_lable[8];
} __attribute__((packed)) FAT_EXTBS_16;

/**
 * @brief FAT Bootsector
 */
typedef struct {
    uint8_t bootjmp[3];
    uint8_t oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sector_count;
    uint8_t table_count;
    uint16_t root_entry_count;
    uint16_t total_sectors_16;
    uint8_t media_type;
    uint16_t table_size_16;
    uint16_t sectors_per_track;
    uint16_t head_side_count;
    uint32_t hidden_sector_count;
    uint32_t total_sectors_32;
    union {
        FAT_EXTBS_16 extbs16;
        FAT_EXTBS_32 extbs32;
    };
} __attribute__((packed)) FAT_BS;

/**
 * @brief Directory entry in the file allocation table
 */
typedef struct {
    char name[11];
    uint8_t attributes;
    uint8_t _reserved;
    uint8_t created_time_tenths;
    uint16_t created_time;
    uint16_t created_date;
    uint16_t accessed_date;
    uint16_t first_cluster_high;
    uint16_t modified_time;
    uint16_t modified_date;
    uint16_t first_cluster_low;
    uint32_t size;
} __attribute__((packed)) FAT_DIR_ENTRY;

/**
 * @brief Long file name entry
 */
typedef struct {
    uint8_t sequence_number;
    char name1[10];
    uint8_t attribute;
    uint8_t type;                   /* Reserved for future use */
    uint8_t dos_checksum;
    char name2[12];
    uint16_t first_cluster;         /* Always zero */
    char name3[4];
} __attribute__((packed)) FAT_LFN_ENTRY;

/**
 * @brief FAT32 Bootsector info
 */
typedef struct {
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sector_count;
    uint8_t num_fats;
    uint32_t sectors_per_fat;
    uint32_t root_dir_first_cluster;
    uint32_t fat_begin_lba;
    uint32_t cluster_begin_lba;
    uint32_t total_sectors;
} FAT32_BS_INFO;

typedef struct {
    uint8_t name[11];
    uint8_t attribute;
    uint32_t cluster_begin;
    uint32_t file_size_bytes;
    uint32_t dir_entry_cluster;
    size_t dir_entry_index;
} FAT32_ENTRY;

typedef struct {
    ATA_DEVICE *device;
    LOCK lock;
    FAT32_BS_INFO bs;
    FAT32_ENTRY entry;
    uint32_t *FAT;
    size_t fat_len;
} FAT32_IDENT;

typedef struct {
    FAT_DIR_ENTRY entry;
    STD_TIME time;
    char name[VFS_MAX_NAME_LEN];
    VFS_INODE *parent;
} FAT32_IDENT_ITEM;