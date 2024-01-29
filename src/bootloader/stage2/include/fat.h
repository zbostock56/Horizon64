#pragma once

#include "disk.h"
#include "mbr.h"
#include <stdint.h>
#include <stdbool.h>

#define SECTOR_SIZE             (512)
#define MAX_PATH_SIZE           (256)
#define MAX_FILE_HANDLES        (10)
#define ROOT_DIRECTORY_HANDLE   (-1)
#define FAT_CACHE_SIZE          (5)

typedef struct {
    /* Extended Boot Record for non-FAT32 */
    uint8_t drive_number;
    uint8_t _reserved;
    uint8_t signiture;
    uint32_t volume_id;
    uint8_t volume_lable[11];
    uint8_t system_id[8];
} __attribute__((packed)) FAT_EXTENDED_BOOT_RECORD;

typedef struct {
    /* Extended Boot Record for FAT32 */
    uint32_t sectors_per_fat;
    uint16_t flags;
    uint16_t fat_version;
    uint32_t root_directory_cluster;
    uint16_t fs_info_sector;
    uint16_t backup_boot_sector;
    FAT_EXTENDED_BOOT_RECORD EBR;
}__attribute__((packed)) FAT32_EXTENDED_BOOT_RECORD;

typedef struct {
    uint8_t name[11];
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
} __attribute__((packed)) FAT_DIRECTORY_ENTRY;

typedef struct {
    int handle;
    bool is_directory;
    uint32_t position;
    uint32_t size;
} FAT_FILE;

enum FAT_Attributes {
    FAT_ATTRIBUTE_READ_ONLY         = 0x01,
    FAT_ATTRIBUTE_HIDDEN            = 0x02,
    FAT_ATTRIBUTE_SYSTEM            = 0x04,
    FAT_ATTRIBUTE_VOLUME_ID         = 0x08,
    FAT_ATTRIBUTE_DIRECTORY         = 0x10,
    FAT_ATTRIBUTE_ARCHIVE           = 0x20,
    FAT_ATTRIBUTE_LFN               = FAT_ATTRIBUTE_READ_ONLY | FAT_ATTRIBUTE_HIDDEN | FAT_ATTRIBUTE_SYSTEM | FAT_ATTRIBUTE_VOLUME_ID
};

typedef struct {
    /* BIOS Parameter Block */
    uint8_t boot_jump_instructions[3];
    uint8_t oem_identifier[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_count;
    uint16_t dir_entry_count;
    uint16_t total_sectors;
    uint8_t media_descriptor_type;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t hidden_sectors;
    uint32_t large_sector_count;

    /* Extended Boot Record */
    union {
        FAT_EXTENDED_BOOT_RECORD EBR_FAT1216;
        FAT32_EXTENDED_BOOT_RECORD EBR_FAT32;
    };
} __attribute__((packed)) FAT_BOOT_SECTOR;

typedef struct {
    uint8_t buffer[SECTOR_SIZE];
    FAT_FILE public;
    bool opened;
    uint32_t first_cluster;
    uint32_t current_cluster;
    uint32_t current_sector_in_cluster;
} FAT_FILE_DATA;

typedef struct {
    union {
        FAT_BOOT_SECTOR boot_sector;
        uint8_t boot_sector_bytes[SECTOR_SIZE];
    } BS;
    FAT_FILE_DATA root_directory;
    FAT_FILE_DATA opened_files[MAX_FILE_HANDLES];
    uint8_t fat_cache[FAT_CACHE_SIZE * SECTOR_SIZE];
    uint32_t fat_cache_position;
} FAT_DATA;

bool FAT_init(PARTITION *);
FAT_FILE *FAT_open(PARTITION *, const char *);
uint32_t FAT_read(PARTITION *, FAT_FILE *, uint32_t, void *);
bool FAT_read_entry(PARTITION *, FAT_FILE *, FAT_DIRECTORY_ENTRY *);
void FAT_close(FAT_FILE *);
uint32_t FAT_cluster_to_lba(uint32_t);
