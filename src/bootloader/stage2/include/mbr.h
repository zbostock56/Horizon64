#pragma once

#include "disk.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    DISK *disk;
    uint32_t partition_offset;
    uint32_t partition_size;
} PARTITION;

typedef struct {
    /* 0x00  1  Drive Attributes (bit 7 set = active/bootable) */
    uint8_t attributes;
    /* 0x01  3  CHS address of partition start */
    uint8_t chs_start[3];
    /* 0x04  1  Partition type */
    uint8_t partition_type;
    /* 0x05  3  CHS address of last partition sector */
    uint8_t chs_end[3];
    /* 0x08  4  LBA of partition start */
    uint32_t lba_start;
    /* 0x0C  4  Number of sectors in partition */
    uint32_t size;
} __attribute__((packed)) MBR_ENTRY;

void MBR_detect_partition(PARTITION *, DISK *, void *);
bool partition_read_sectors(PARTITION *, uint32_t, uint8_t, void *);