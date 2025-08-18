/**
 * @file ata_str.h
 * @author Zack Bostock
 * @brief Structures related to ATA devices
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>

typedef struct {
    uint16_t base;  /* IO Base */
    uint16_t ctrl;  /* Control base */
    uint16_t bmide; /* Bus Master IDE */
    uint16_t nien;  /* nIEN (No Interrupt) */
} IDE_CHANNEL_REGS;

typedef struct {
    uint8_t  reserved;      /* 0 (empty) or 1 (this drive really exists) */
    uint8_t  channel;       /* 0 (primary channel) or 1 (secondary channel) */
    uint8_t  drive;         /* 0 (Master Drive) or 1 (Slave Drive) */
    uint16_t type;          /* 0: ATA, 1: ATAPI */
    uint16_t signature;     /* Drive signature */
    uint16_t capabilities;  /* Features */
    uint32_t command_sets;  /* Command sets supported */
    uint32_t size;          /* Size in sectors */
    uint8_t  model[41];     /* Model string */
} IDE_DEVICE;

typedef struct {
    uint8_t  status;
    uint8_t  chs_start[3];
    uint8_t  type;
    uint8_t  chs_end[3];
    uint32_t lba_start;
    uint32_t sector_count;
} __attribute__((packed)) PARTITION;

typedef struct {
    uint16_t flags;
    uint16_t unused1[9];
    char     serial[20];
    uint16_t unused2[3];
    char     firmware[8];
    char     model[40];
    uint16_t sectors_per_int;
    uint16_t unused3;
    uint16_t capabilities[2];
    uint16_t unused4[2];
    uint16_t valid_ext_data;
    uint16_t unused5[5];
    uint16_t size_of_rw_mult;
    uint32_t sectors_28;
    uint16_t unused6[38];
    uint64_t sectors_48;
    uint16_t unused7[152];
} __attribute__((packed)) ATA_IDENTIFY;

typedef struct {
    uintptr_t offset;
    uint16_t bytes;
    uint16_t last;
} PRDT;

typedef struct {
    int io_base;
    int control;
    int slave;
    int is_atapi;
    ATA_IDENTIFY identity;
    PRDT *dma_prdt;
    uintptr_t dma_prdt_phys;
    uint8_t *dma_start;
    uintptr_t dma_start_phys;
    uint32_t bar4;
    uint32_t atapi_lba;
    uint32_t atapi_sector_size;
} ATA_DEVICE;

typedef struct {
    uint8_t boostrap[446];
    PARTITION partitions[4];
    uint8_t signature[2];
} __attribute__((packed)) MBR;

typedef union {
    uint8_t command_bytes[12];
    uint16_t command_words[6];
} ATAPI_COMMAND;

/**
 * @brief ATA device statistics structure
 *
 * Contains key information and statistics about an ATA/ATAPI device
 */
typedef struct {
    /* Device type information */
    int is_atapi;                    // 1 if ATAPI device, 0 if ATA

    /* Capacity information */
    uint32_t sectors_28;             // 28-bit LBA sector count
    uint64_t sectors_48;             // 48-bit LBA sector count
    uint64_t max_offset;             // Maximum addressable offset

    /* ATAPI-specific information (only valid if is_atapi == 1) */
    uint32_t atapi_lba;              // ATAPI LBA capacity
    uint32_t atapi_sector_size;      // ATAPI sector size (usually 2048)

    /* Device identification strings  */
    char model[41];                  // Model string (40 chars + null terminator)
    char serial[21];                 // Serial number (20 chars + null terminator)
} ATA_DEVICE_STATS;