/**
 * @file ata.c
 * @author Zack Bostock (improved)
 * @brief Functionality related to ATA devices
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <common/kprint.h>
#include <common/lock.h>

#include <sys/pci.h>
#include <sys/cpu.h>
#include <sys/asm.h>

#include <dev/storage/ata.h>

#include <fs/vfs.h>

#define ATA_SECTOR_SIZE     (512)
#define ATA_POLL_YES        (1)
#define ATA_POLL_NO         (0)
#define ATA_MAX_RETRIES     (3)
#define ATA_TIMEOUT_CYCLES  (0x1000)

/* Error codes for better error reporting */
typedef enum {
    ATA_OK = 0,
    ATA_ERR_TIMEOUT,
    ATA_ERR_DEVICE_FAULT,
    ATA_ERR_BAD_SECTOR,
    ATA_ERR_MEDIA_CHANGED,
    ATA_ERR_ID_NOT_FOUND,
    ATA_ERR_MEDIA_CHANGE_REQUEST,
    ATA_ERR_COMMAND_ABORTED,
    ATA_ERR_TRACK_ZERO_NOT_FOUND,
    ATA_ERR_ADDRESS_MARK_NOT_FOUND,
    ATA_ERR_INVALID_PARAMS
} ATA_ERROR_CODE;

static char ata_drive_char = 'a';
static int cdrom_number = 0;

static ATA_DEVICE ata_primary_master = {
    .io_base = 0x1F0,
    .control = 0x3F6,
    .slave = 0
};

static ATA_DEVICE ata_primary_slave = {
    .io_base = 0x1F0,
    .control = 0x3F6,
    .slave = 1
};

static ATA_DEVICE ata_secondary_master = {
    .io_base = 0x170,
    .control = 0x376,
    .slave = 0
};

static ATA_DEVICE ata_secondary_slave = {
    .io_base = 0x170,
    .control = 0x376,
    .slave = 1
};

/* Mutex for thread safety */
static LOCK ata_lock = LOCK_NEW;

/**
 * @brief Decode ATA error register
 *
 * @param error_reg Error register value
 * @return ATA_ERROR_CODE Decoded error code
 */
static ATA_ERROR_CODE ata_decode_error(uint8_t error_reg) {
    if (error_reg & (1 << 7)) return ATA_ERR_BAD_SECTOR;
    if (error_reg & (1 << 6)) return ATA_ERR_MEDIA_CHANGED;
    if (error_reg & (1 << 4)) return ATA_ERR_ID_NOT_FOUND;
    if (error_reg & (1 << 3)) return ATA_ERR_MEDIA_CHANGE_REQUEST;
    if (error_reg & (1 << 2)) return ATA_ERR_COMMAND_ABORTED;
    if (error_reg & (1 << 1)) return ATA_ERR_TRACK_ZERO_NOT_FOUND;
    if (error_reg & (1 << 0)) return ATA_ERR_ADDRESS_MARK_NOT_FOUND;
    return ATA_OK;
}

/**
 * @brief Print human-readable error message
 *
 * @param error Error code
 */
static void ata_print_error(ATA_ERROR_CODE error) {
    switch (error) {
        case ATA_ERR_TIMEOUT:
            kloge("ATA: Operation timed out\n");
            break;
        case ATA_ERR_DEVICE_FAULT:
            kloge("ATA: Device fault\n");
            break;
        case ATA_ERR_BAD_SECTOR:
            kloge("ATA: Bad sector\n");
            break;
        case ATA_ERR_MEDIA_CHANGED:
            kloge("ATA: Media changed\n");
            break;
        case ATA_ERR_ID_NOT_FOUND:
            kloge("ATA: ID not found\n");
            break;
        case ATA_ERR_MEDIA_CHANGE_REQUEST:
            kloge("ATA: Media change request\n");
            break;
        case ATA_ERR_COMMAND_ABORTED:
            kloge("ATA: Command aborted\n");
            break;
        case ATA_ERR_TRACK_ZERO_NOT_FOUND:
            kloge("ATA: Track zero not found\n");
            break;
        case ATA_ERR_ADDRESS_MARK_NOT_FOUND:
            kloge("ATA: Address mark not found\n");
            break;
        case ATA_ERR_INVALID_PARAMS:
            kloge("ATA: Invalid parameters\n");
            break;
        default:
            kloge("ATA: Unknown error code %d\n", error);
    }
}

/**
 * @brief Waiting for a specific amount of time
 *
 * @param dev Device to wait on
 */
static void ata_io_wait(ATA_DEVICE *dev) {
    if (!dev) return;

    /* Delay 400 nanoseconds for BSY to be set */
    /* Reading the alternate status port wastes 100ns, do it 4x times */
    for (int i = 0; i < 4; i++) {
        inb(dev->control);
    }
}

/**
 * @brief Soft reset with timing
 *
 * @param dev Device to soft reset
 * @return int 0 on success, -1 on failure
 */
static int ata_soft_reset(ATA_DEVICE *dev) {
    if (!dev) return -1;

    klogd("ATA: Soft resetting device at 0x%x\n", dev->io_base);

    /* Set SRST bit */
    outb(dev->control, 0x04);
    // ata_io_wait(dev);

    /* Clear SRST bit */
    outb(dev->control, 0x00);

    /* Wait for reset to complete - up to 2 seconds */
    uint32_t timeout = ATA_TIMEOUT_CYCLES;
    uint8_t status;

    do {
        ata_io_wait(dev);
        status = inb(dev->control);
        timeout--;
    } while ((status & ATA_SR_BSY) && timeout > 0);

    if (timeout == 0) {
        kloge("ATA: Soft reset timeout\n");
        return -1;
    }

    klogd("ATA: Soft reset completed\n");
    return 0;
}

/**
 * @brief Polling function with timeout functionality
 *
 * @param dev Device to poll
 * @param advanced_check Check and see if any errors occurred
 * @return int 0 on success, negative on error
 */
static int ata_poll(ATA_DEVICE *dev, int advanced_check) {
    if (!dev) return -1;

    ata_io_wait(dev);

    uint32_t timeout = ATA_TIMEOUT_CYCLES;
    uint8_t status;

    /* Wait for BSY to clear */
    do {
        status = inb(dev->control);
        timeout--;
        if (timeout == 0) {
            kloge("ATA: Polling timeout waiting for BSY to clear\n");
            return -ATA_ERR_TIMEOUT;
        }
    } while (status & ATA_SR_BSY);

    if (advanced_check) {
        timeout = ATA_TIMEOUT_CYCLES;

        while (timeout > 0) {
            status = inb(dev->io_base + ATA_REG_STATUS);

            /* Check for errors */
            if (status & ATA_SR_ERR) {
                uint8_t error_reg = inb(dev->io_base + ATA_REG_ERROR);
                ATA_ERROR_CODE error = ata_decode_error(error_reg);
                ata_print_error(error);
                return -error;
            }

            /* Check for device fault */
            if (status & ATA_SR_DF) {
                kloge("ATA: Device fault detected\n");
                return -ATA_ERR_DEVICE_FAULT;
            }

            /* Check if data is ready */
            if (status & ATA_SR_DRQ) {
                return 0; /* Success */
            }

            /* Check if device is ready (for non-data commands) */
            if (!advanced_check && (status & ATA_SR_DRDY)) {
                return 0;
            }

            timeout--;
            ata_io_wait(dev);
        }

        kloge("ATA: Polling timeout waiting for DRQ\n");
        return -ATA_ERR_TIMEOUT;
    }

    return 0;
}

/**
 * @brief Validate LBA parameters
 *
 * @param dev Device
 * @param lba LBA address
 * @param sector_count Number of sectors
 * @return int 0 if valid, -1 if invalid
 */
static int ata_validate_lba(ATA_DEVICE *dev, uint32_t lba, uint8_t sector_count) {
    if (!dev || sector_count == 0) return -1;

    uint64_t max_sectors = dev->identity.sectors_48;
    if (max_sectors == 0) {
        max_sectors = dev->identity.sectors_28;
    }

    if (max_sectors == 0) {
        kloge("ATA: Device has no valid sector count\n");
        return -1;
    }

    if (lba >= max_sectors || (uint64_t)lba + sector_count > max_sectors) {
        kloge("ATA: LBA %u + %u sectors exceeds device capacity %llu\n",
              lba, sector_count, max_sectors);
        return -1;
    }

    /* Check for 28-bit LBA limits */
    if (lba >= (1ULL << 28)) {
        kloge("ATA: LBA %u exceeds 28-bit addressing limit\n", lba);
        return -1;
    }

    return 0;
}

/**
 * @brief Helper to find the max offset of an ATA device
 *
 * @param dev Device to find max offset of
 * @return uint64_t Max device offset, 0 if invalid
 */
static uint64_t ata_max_offset(ATA_DEVICE *dev) {
    if (!dev) return 0;

    uint64_t sectors = dev->identity.sectors_48;
    if (sectors == 0) {
        /* Fall back to sectors_28 */
        sectors = dev->identity.sectors_28;
    }

    return sectors * ATA_SECTOR_SIZE;
}

/**
 * @brief Device initialization
 *
 * @param dev Device to initialize
 * @return int 1 if success, 0 if failure
 */
static int init_ata_device(ATA_DEVICE *dev) {
    if (!dev) return 0;

    klogi("ATA: Initializing device at 0x%x (slave: %d)\n", dev->io_base, dev->slave);

    uint16_t bus = dev->io_base;
    uint8_t slave = dev->slave;

    /* Clear interrupts */
    outb(dev->control, 0x02);
    ata_io_wait(dev);

    /* Select drive */
    outb(bus + ATA_REG_HDDEVSEL, 0xA0 | (slave << 4));
    ata_io_wait(dev);

    /* Clear registers */
    outb(bus + ATA_REG_SECCOUNT0, 0);
    outb(bus + ATA_REG_LBA0, 0);
    outb(bus + ATA_REG_LBA1, 0);
    outb(bus + ATA_REG_LBA2, 0);

    /* Send IDENTIFY command */
    outb(bus + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    ata_io_wait(dev);

    /* Check if drive exists */
    uint8_t status = inb(bus + ATA_REG_STATUS);
    if (status == 0) {
        klogd("ATA: No drive detected at 0x%x\n", dev->io_base);
        return 0;
    }

    /* Wait for command completion with timeout */
    uint32_t timeout = ATA_TIMEOUT_CYCLES;
    while (timeout > 0) {
        status = inb(bus + ATA_REG_STATUS);

        if (status & ATA_SR_ERR) {
            klogd("ATA: Drive error during IDENTIFY\n");
            return 0;
        }

        if (!(status & ATA_SR_BSY)) {
            if (status & ATA_SR_DRQ) {
                break; /* Ready to read data */
            }
            if (status & ATA_SR_DRDY) {
                klogd("ATA: Drive ready but no data - not an ATA device\n");
                return 0;
            }
        }

        timeout--;
        ata_io_wait(dev);
    }

    if (timeout == 0) {
        kloge("ATA: IDENTIFY command timed out\n");
        return 0;
    }

    /* Check cylinder registers for device type */
    uint8_t mid = inb(bus + ATA_REG_LBA1);
    uint8_t hi = inb(bus + ATA_REG_LBA2);

    if (mid != 0 || hi != 0) {
        klogd("ATA: Device signature indicates non-ATA device (%02x %02x)\n", mid, hi);
        return 0;
    }

    /* Wait for DRQ with timeout */
    timeout = ATA_TIMEOUT_CYCLES;
    while (timeout > 0) {
        status = inb(bus + ATA_REG_STATUS);
        if (status & ATA_SR_ERR) {
            kloge("ATA: Error waiting for DRQ\n");
            return 0;
        }
        if (status & ATA_SR_DRQ) {
            break;
        }
        timeout--;
        ata_io_wait(dev);
    }

    if (timeout == 0) {
        kloge("ATA: Timeout waiting for DRQ\n");
        return 0;
    }

    /* Read identification data */
    uint16_t *buf = (uint16_t *)(&dev->identity);
    insw(bus + ATA_REG_DATA, buf, 256);
    dev->is_atapi = 0;

    /* Fix byte order for model string */
    uint8_t *ptr = (uint8_t *)(dev->identity.model);
    for (int i = 0; i < 40; i += 2) {
        if (i + 1 < 40) {
            uint8_t temp = ptr[i + 1];
            ptr[i + 1] = ptr[i];
            ptr[i] = temp;
        }
    }
    ptr[39] = '\0';

    /* Fix byte order for serial number */
    ptr = (uint8_t *)(dev->identity.serial);
    for (int i = 0; i < 20; i += 2) {
        if (i + 1 < 20) {
            uint8_t temp = ptr[i + 1];
            ptr[i + 1] = ptr[i];
            ptr[i] = temp;
        }
    }
    ptr[19] = '\0';

    /* Trim whitespace from model name */
    for (int i = 39; i >= 0 && (ptr[i] == ' ' || ptr[i] == '\0'); i--) {
        ((uint8_t*)dev->identity.model)[i] = '\0';
    }

    klogi("ATA: Device initialized successfully\n");
    klogi("  Model: %s\n", dev->identity.model);
    klogi("  Serial: %s\n", dev->identity.serial);
    klogi("  Sectors (48-bit): %llu\n", dev->identity.sectors_48);
    klogi("  Sectors (28-bit): %u\n", dev->identity.sectors_28);
    klogi("  Capacity: %llu bytes\n", ata_max_offset(dev));

    /* Enable interrupts */
    outb(dev->control, 0x00);

    return 1;
}

/**
 * @brief PIO read with retries and validation
 *
 * @param dev Device to read from
 * @param lba LBA number
 * @param sector_count Number of sectors
 * @param target Target buffer to copy data into
 * @return int 0 on success, negative on error
 */
int ata_pio_read28(ATA_DEVICE *dev, uint32_t lba, uint8_t sector_count,
                   uint8_t *target) {
    if (!dev || !target || sector_count == 0) {
        return -ATA_ERR_INVALID_PARAMS;
    }

    /* Validate parameters */
    if (ata_validate_lba(dev, lba, sector_count) != 0) {
        return -ATA_ERR_INVALID_PARAMS;
    }

    /* Thread safety */
    LOCK_LOCK(&ata_lock);

    int result = 0;
    uint16_t bus = dev->io_base;
    uint8_t slave = dev->slave;

    for (int retry = 0; retry < ATA_MAX_RETRIES; retry++) {
        if (retry > 0) {
            klogw("ATA: Read retry %d for LBA %u\n", retry, lba);
            ata_soft_reset(dev);
        }

        ata_io_wait(dev);

        /* Select drive and set LBA */
        outb(bus + ATA_REG_HDDEVSEL, 0xE0 | (slave << 4) | ((lba & 0x0F000000) >> 24));
        ata_io_wait(dev);

        /* Set parameters */
        outb(bus + ATA_REG_ERROR, 0x00);
        outb(bus + ATA_REG_SECCOUNT0, sector_count);
        outb(bus + ATA_REG_LBA0, (lba & 0x000000FF));
        outb(bus + ATA_REG_LBA1, (lba & 0x0000FF00) >> 8);
        outb(bus + ATA_REG_LBA2, (lba & 0x00FF0000) >> 16);

        /* Send command */
        outb(bus + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

        /* Read sectors */
        for (uint32_t i = 0; i < sector_count; i++) {
            int poll_result = ata_poll(dev, ATA_POLL_YES);
            if (poll_result < 0) {
                result = poll_result;
                break;
            }

            /* Transfer the data */
            insw(bus + ATA_REG_DATA, (uint16_t*)target, 256);
            target += ATA_SECTOR_SIZE;
            ata_io_wait(dev);
        }

        if (result == 0) {
            /* Final status check */
            int poll_result = ata_poll(dev, ATA_POLL_NO);
            if (poll_result >= 0) {
                break; /* Success */
            }
            result = poll_result;
        }
    }

    UNLOCK_LOCK(&ata_lock);

    if (result < 0) {
        kloge("ATA: Read failed after %d retries\n", ATA_MAX_RETRIES);
    }

    return result;
}

/**
 * @brief PIO write with retries and validation
 *
 * @param dev Device to write to
 * @param lba LBA number
 * @param sector_count Number of sectors
 * @param src Buffer with bytes to write
 * @return int 0 on success, negative on error
 */
int ata_pio_write28(ATA_DEVICE *dev, uint32_t lba, uint8_t sector_count,
                    const uint8_t *src) {
    if (!dev || !src || sector_count == 0) {
        return -ATA_ERR_INVALID_PARAMS;
    }

    /* Validate parameters */
    if (ata_validate_lba(dev, lba, sector_count) != 0) {
        return -ATA_ERR_INVALID_PARAMS;
    }

    /* Thread safety */
    LOCK_LOCK(&ata_lock);

    int result = 0;
    uint16_t bus = dev->io_base;
    uint8_t slave = dev->slave;

    for (int retry = 0; retry < ATA_MAX_RETRIES; retry++) {
        if (retry > 0) {
            klogw("ATA: Write retry %d for LBA %u\n", retry, lba);
            ata_soft_reset(dev);
        }

        ata_io_wait(dev);

        /* Select drive and set LBA */
        outb(bus + ATA_REG_HDDEVSEL, 0xE0 | (slave << 4) | ((lba & 0x0F000000) >> 24));
        ata_io_wait(dev);

        /* Set parameters */
        outb(bus + ATA_REG_ERROR, 0x00);
        outb(bus + ATA_REG_SECCOUNT0, sector_count);
        outb(bus + ATA_REG_LBA0, (lba & 0x000000FF));
        outb(bus + ATA_REG_LBA1, (lba & 0x0000FF00) >> 8);
        outb(bus + ATA_REG_LBA2, (lba & 0x00FF0000) >> 16);

        /* Send command */
        outb(bus + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
        ata_io_wait(dev);

        /* Write sectors */
        for (uint32_t i = 0; i < sector_count; i++) {
            int poll_result = ata_poll(dev, ATA_POLL_YES);
            if (poll_result < 0) {
                result = poll_result;
                break;
            }

            /* Transfer data */
            const uint16_t *data = (const uint16_t*)src;
            for (size_t j = 0; j < 256; j++) {
                outw(bus + ATA_REG_DATA, data[j]);
            }

            src += ATA_SECTOR_SIZE;
            ata_io_wait(dev);
        }

        if (result == 0) {
            /* Flush cache */
            outb(bus + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
            int poll_result = ata_poll(dev, ATA_POLL_NO);
            if (poll_result >= 0) {
                break; /* Success */
            }
            result = poll_result;
        }
    }

    UNLOCK_LOCK(&ata_lock);

    if (result < 0) {
        kloge("ATA: Write failed after %d retries\n", ATA_MAX_RETRIES);
    }

    return result;
}

/**
 * @brief Partition map reading with validation
 *
 * @param dev Device to read from
 * @param dev_name Name of device
 * @return STATUS SYS_OK on success, SYS_ERR on failure
 */
STATUS ata_read_partition_map(ATA_DEVICE *dev, const char *dev_name) {
    if (!dev || !dev_name) {
        kloge("ATA: Invalid parameters for partition map reading\n");
        return SYS_ERR;
    }

    MBR mbr;
    int result = ata_pio_read28(dev, 0, 1, (uint8_t *)&mbr);
    if (result < 0) {
        kloge("ATA: Failed to read MBR from %s\n", dev_name);
        return SYS_ERR;
    }

    if (mbr.signature[0] != 0x55 || mbr.signature[1] != 0xAA) {
        kloge("ATA: Invalid MBR signature on %s: 0x%02x 0x%02x (expected 0x55 0xAA)\n",
              dev_name, mbr.signature[0], mbr.signature[1]);
        return SYS_ERR;
    }

    klogi("ATA: Valid partition table found on %s\n", dev_name);

    int partitions_found = 0;
    for (int i = 0; i < 4; i++) {
        /* Check for valid partition */
        if (mbr.partitions[i].type == 0) {
            continue; /* Empty partition slot */
        }

        klogi("ATA: Partition %d: type 0x%02x, LBA start %u, sectors %u\n",
              i, mbr.partitions[i].type, mbr.partitions[i].lba_start,
              mbr.partitions[i].sector_count);

        /* FAT32 partition types */
        if (mbr.partitions[i].type == 0x0B ||
            mbr.partitions[i].type == 0x0C ||
            mbr.partitions[i].type == 0x1C) {

            char part_name[8];
            char part_path[VFS_MAX_PATH_LEN];

            snprintf(part_name, sizeof(part_name), "%d", i);
            snprintf(part_path, sizeof(part_path), "/disk/%s", part_name);

            if (vfs_path_to_node(part_path, CREATE, VFS_DIRECTORY)) {
                if (vfs_mount(dev_name, part_path, "fat32") == 0) {
                    klogi("ATA: Successfully mounted FAT32 partition %d at %s\n", i, part_path);
                    partitions_found++;
                } else {
                    kloge("ATA: Failed to mount partition %d\n", i);
                }
            } else {
                kloge("ATA: Failed to create directory node for partition %d\n", i);
            }
        }
    }

    if (partitions_found > 0) {
        klogi("ATA: Successfully processed %d partitions on %s\n", partitions_found, dev_name);
        return SYS_OK;
    } else {
        klogw("ATA: No supported partitions found on %s\n", dev_name);
        return SYS_ERR;
    }
}

/**
 * @brief Enhanced ATAPI device initialization
 *
 * @param dev ATAPI device to initialize
 * @return int 0 if failure, 1 if success
 */
static int init_atapi_device(ATA_DEVICE *dev) {
    if (!dev) return 0;

    klogi("ATA: Initializing ATAPI device at 0x%x\n", dev->io_base);
    dev->is_atapi = 1;

    uint16_t bus = dev->io_base;
    uint8_t slave = dev->slave;

    /* Reset and setup */
    outb(dev->io_base + 1, 1);
    outb(dev->control, 0);
    ata_io_wait(dev);

    /* Select drive */
    outb(bus + ATA_REG_HDDEVSEL, 0xA0 | (slave << 4));
    ata_io_wait(dev);

    /* Clear registers */
    outb(bus + ATA_REG_SECCOUNT0, 0);
    outb(bus + ATA_REG_LBA0, 0);
    outb(bus + ATA_REG_LBA1, 0);
    outb(bus + ATA_REG_LBA2, 0);

    /* Send IDENTIFY PACKET command */
    outb(bus + ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
    ata_io_wait(dev);

    /* Wait for response with timeout */
    uint32_t timeout = ATA_TIMEOUT_CYCLES;
    uint8_t status;

    do {
        status = inb(bus + ATA_REG_STATUS);
        timeout--;
        if (timeout == 0) {
            klogd("ATA: ATAPI device detection timeout\n");
            return 0;
        }
    } while (status & ATA_SR_BSY);

    if (status == 0) {
        klogd("ATA: No ATAPI device detected\n");
        return 0;
    }

    /* Check device signature */
    uint8_t mid = inb(bus + ATA_REG_LBA1);
    uint8_t hi = inb(bus + ATA_REG_LBA2);

    if (mid != 0x14 || hi != 0xEB) {
        klogd("ATA: Invalid ATAPI signature: %02x %02x\n", mid, hi);
        return 0;
    }

    /* Wait for DRQ or ERR */
    timeout = ATA_TIMEOUT_CYCLES;
    while (timeout > 0 && !(status & (ATA_SR_ERR | ATA_SR_DRQ))) {
        status = inb(bus + ATA_REG_STATUS);
        timeout--;
        ata_io_wait(dev);
    }

    if (status & ATA_SR_ERR) {
        klogd("ATA: ATAPI device error during identification\n");
        return 0;
    }

    if (timeout == 0) {
        kloge("ATA: ATAPI identification timeout\n");
        return 0;
    }

    /* Read identification data */
    uint16_t *buf = (uint16_t *)(&dev->identity);
    insw(bus + ATA_REG_DATA, buf, 256);

    /* Fix byte order for strings */
    uint8_t *ptr = (uint8_t *)(dev->identity.model);
    for (int i = 0; i < 40; i += 2) {
        if (i + 1 < 40) {
            uint8_t temp = ptr[i + 1];
            ptr[i + 1] = ptr[i];
            ptr[i] = temp;
        }
    }
    ptr[39] = '\0';

    ptr = (uint8_t *)(dev->identity.serial);
    for (int i = 0; i < 20; i += 2) {
        if (i + 1 < 20) {
            uint8_t temp = ptr[i + 1];
            ptr[i + 1] = ptr[i];
            ptr[i] = temp;
        }
    }
    ptr[19] = '\0';

    klogi("ATA: ATAPI device initialized\n");
    klogi("  Model: %s\n", dev->identity.model);
    klogi("  Serial: %s\n", dev->identity.serial);

    /* Try to detect medium using READ CAPACITY command */
    ATAPI_COMMAND cmd = {0};
    cmd.command_bytes[0] = 0x25;  /* READ CAPACITY */
    /* Other bytes are already zero */

    outb(bus + ATA_REG_FEATURES, 0x00);
    outb(bus + ATA_REG_LBA1, 0x08);  /* Byte count low */
    outb(bus + ATA_REG_LBA2, 0x00);  /* Byte count high */
    outb(bus + ATA_REG_COMMAND, ATA_CMD_PACKET);

    /* Wait for device ready */
    timeout = ATA_TIMEOUT_CYCLES;
    while (timeout > 0) {
        status = inb(bus + ATA_REG_STATUS);
        if (status & ATA_SR_ERR) {
            klogw("ATA: ATAPI packet command error\n");
            goto atapi_no_medium;
        }
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) {
            break;
        }
        timeout--;
        ata_io_wait(dev);
    }

    if (timeout == 0) {
        klogw("ATA: ATAPI packet command timeout\n");
        goto atapi_no_medium;
    }

    /* Send packet command */
    for (int i = 0; i < 6; i++) {
        outw(bus + ATA_REG_DATA, cmd.command_words[i]);
    }

    /* Wait for data ready */
    timeout = ATA_TIMEOUT_CYCLES;
    while (timeout > 0) {
        status = inb(bus + ATA_REG_STATUS);
        if (status & ATA_SR_ERR) {
            klogw("ATA: ATAPI READ capacity error - no medium?\n");
            goto atapi_no_medium;
        }
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) {
            break;
        }
        timeout--;
        ata_io_wait(dev);
    }

    if (timeout == 0) {
        klogw("ATA: ATAPI read capacity timeout\n");
        goto atapi_no_medium;
    }

    /* Read capacity data */
    uint16_t data[4];
    insw(bus + ATA_REG_DATA, data, 4);

    uint32_t lba, block_size;
    memcpy(&lba, &data[0], sizeof(uint32_t));
    memcpy(&block_size, &data[2], sizeof(uint32_t));

    /* Convert from big-endian */
    lba = ((lba & 0xFF) << 24) | ((lba & 0xFF00) << 8) |
          ((lba & 0xFF0000) >> 8) | ((lba & 0xFF000000) >> 24);
    block_size = ((block_size & 0xFF) << 24) | ((block_size & 0xFF00) << 8) |
                 ((block_size & 0xFF0000) >> 8) | ((block_size & 0xFF000000) >> 24);

    dev->atapi_lba = lba;
    dev->atapi_sector_size = block_size;

    if (lba == 0) {
        klogw("ATA: ATAPI device has no medium\n");
        return 1;  /* Device exists but no medium */
    }

    klogi("ATA: ATAPI medium detected - LBA: %u, Block size: %u bytes\n", lba, block_size);
    return 1;

atapi_no_medium:
    dev->atapi_lba = 0;
    dev->atapi_sector_size = 0;
    return 1;  /* Device exists but no medium or error reading capacity */
}

/**
 * @brief Enhanced device detection with better error handling
 *
 * @param dev Device to detect what it is
 * @return int 0 if unknown, 1 if ATA device, 2 if ATAPI device, -1 if error
 */
static int ata_device_detect(ATA_DEVICE *dev) {
    if (!dev) return -1;

    klogd("ATA: Detecting device at 0x%x (slave: %d)\n", dev->io_base, dev->slave);

    /* Soft reset first */
    if (ata_soft_reset(dev) != 0) {
        kloge("ATA: Failed to reset device at 0x%x\n", dev->io_base);
        return 0;
    }

    /* Select drive */
    outb(dev->io_base + ATA_REG_HDDEVSEL, 0xA0 | (dev->slave << 4));
    ata_io_wait(dev);

    /* Read device signature */
    uint8_t cl = inb(dev->io_base + ATA_REG_LBA1);  /* CYL_LO */
    uint8_t ch = inb(dev->io_base + ATA_REG_LBA2);  /* CYL_HI */

    klogi("ATA: Device signature: 0x%02x 0x%02x\n", cl, ch);

    if (cl == 0xFF && ch == 0xFF) {
        klogd("ATA: No device detected\n");
        return 0;
    } else if ((cl == 0x00 && ch == 0x00) || (cl == 0x3C && ch == 0xC3)) {
        /* Parallel ATA device, or emulated SATA */
        klogi("ATA: ATA/SATA device detected\n");

        if (!init_ata_device(dev)) {
            klogw("ATA: Failed to initialize ATA device\n");
            return 0;
        }

        /* Create device node */
        char dev_name[64];
        snprintf(dev_name, sizeof(dev_name), "/dev/hd%c", ata_drive_char);

        VFS_TNODE *tnode = vfs_path_to_node(dev_name, CREATE, VFS_BLOCK_DEV);
        if (!tnode) {
            kloge("ATA: Failed to create device node %s\n", dev_name);
            return 0;
        }

        VFS_INODE *inode = vfs_alloc_inode(VFS_BLOCK_DEV, 0777, 0, NULL, tnode);
        if (!inode) {
            kloge("ATA: Failed to allocate inode for %s\n", dev_name);
            return 0;
        }

        tnode->inode = inode;
        inode->ident = (void *)dev;

        /* Read and mount partitions */
        if (ata_read_partition_map(dev, dev_name) == SYS_OK) {
            klogi("ATA: Successfully processed partitions for %s\n", dev_name);
        } else {
            klogw("ATA: Failed to process partitions for %s\n", dev_name);
        }

        ata_drive_char++;
        return 1;

    } else if ((cl == 0x14 && ch == 0xEB) || (cl == 0x69 && ch == 0x96)) {
        /* ATAPI device */
        klogi("ATA: ATAPI device detected\n");

        if (!init_atapi_device(dev)) {
            klogw("ATA: Failed to initialize ATAPI device\n");
            return 0;
        }

        /* Create device node */
        char dev_name[64];
        snprintf(dev_name, sizeof(dev_name), "/dev/cdrom%d", cdrom_number);

        VFS_TNODE *tnode = vfs_path_to_node(dev_name, CREATE, VFS_BLOCK_DEV);
        if (!tnode) {
            kloge("ATA: Failed to create ATAPI device node %s\n", dev_name);
            return 0;
        }

        VFS_INODE *inode = vfs_alloc_inode(VFS_BLOCK_DEV, 0777, 0, NULL, tnode);
        if (!inode) {
            kloge("ATA: Failed to allocate inode for ATAPI device %s\n", dev_name);
            return 0;
        }

        tnode->inode = inode;
        inode->ident = (void *)dev;

        cdrom_number++;
        return 2;
    } else {
        klogw("ATA: Unknown device signature: 0x%02x 0x%02x\n", cl, ch);
        return 0;
    }
}

/**
 * @brief Get device statistics
 *
 * @param dev Device to get stats for
 * @param stats Output statistics structure
 * @return int 0 on success, -1 on error
 */
int ata_get_device_stats(ATA_DEVICE *dev, ATA_DEVICE_STATS *stats) {
    if (!dev || !stats) return -1;

    memset(stats, 0, sizeof(ATA_DEVICE_STATS));

    stats->is_atapi = dev->is_atapi;
    stats->sectors_28 = dev->identity.sectors_28;
    stats->sectors_48 = dev->identity.sectors_48;
    stats->max_offset = ata_max_offset(dev);

    if (dev->is_atapi) {
        stats->atapi_lba = dev->atapi_lba;
        stats->atapi_sector_size = dev->atapi_sector_size;
    }

    strncpy(stats->model, (char*)dev->identity.model, sizeof(stats->model) - 1);
    strncpy(stats->serial, (char*)dev->identity.serial, sizeof(stats->serial) - 1);

    return 0;
}

/**
 * @brief Test device functionality
 *
 * @param dev Device to test
 * @return int 0 on success, negative on error
 */
int ata_test_device(ATA_DEVICE *dev) {
    if (!dev || dev->is_atapi) {
        return -ATA_ERR_INVALID_PARAMS;
    }

    klogi("ATA: Testing device functionality\n");

    /* Test read of first sector */
    uint8_t test_buffer[ATA_SECTOR_SIZE];
    int result = ata_pio_read28(dev, 0, 1, test_buffer);

    if (result == 0) {
        klogi("ATA: Device test passed\n");
    } else {
        kloge("ATA: Device test failed with error %d\n", -result);
    }

    return result;
}

/**
 * @brief Main initialization function for ATA devices
 */
void init_ata() {
    klogs("ATA: starting...\n");

    /* Reset global counters */
    ata_drive_char = 'a';
    cdrom_number = 0;

    /* Detect and initialize all possible devices */
    int devices_found = 0;

    int result = ata_device_detect(&ata_primary_master);
    if (result > 0) devices_found++;

    result = ata_device_detect(&ata_primary_slave);
    if (result > 0) devices_found++;

    result = ata_device_detect(&ata_secondary_master);
    if (result > 0) devices_found++;

    result = ata_device_detect(&ata_secondary_slave);
    if (result > 0) devices_found++;

    klogi("ATA: Initialization complete - %d devices found\n", devices_found);

    if (devices_found == 0) {
        klogw("ATA: No ATA/ATAPI devices detected\n");
    }
    klogs("ATA: finished...\n");
}