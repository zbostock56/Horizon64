/**
 * @file ata.c
 * @author Zack Bostock
 * @brief Functionality related to ATA devices
 * 
 * @copyright Copyright (c) 2024
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

//static LOCK ata_lock;

/**
 * @brief Helper for waiting for a specific amount of time
 *
 * @param dev Device to wait on
 */
static void ata_io_wait(ATA_DEVICE *dev) {
    /* Delay 400 nanoseconds for BSY to be set */
    /* Reading the alternate status port wastes 100ns, do it 4x times */
    inb(dev->io_base + ATA_REG_ALTSTATUS);
    inb(dev->io_base + ATA_REG_ALTSTATUS);
    inb(dev->io_base + ATA_REG_ALTSTATUS);
    inb(dev->io_base + ATA_REG_ALTSTATUS);
}

/**
 * @brief Soft resets the device
 *
 * @param dev Device to soft reset
 */
static void ata_soft_reset(ATA_DEVICE *dev) {
    klogd("%s: Sending 0x04\n", __func__);
    outb(dev->control, 0x04);
    klogd("%s: Sending 0x00\n", __func__);
    outb(dev->control, 0x00);
    klogd("%s: done...\n", __func__);
}

/**
 * @brief Polling function to wait for the device to not be busy
 *
 * @param dev Device to poll
 * @param advanced_check Check and see if any errors occured
 */
static void ata_poll(ATA_DEVICE *dev, int advanced_check) {
    ata_io_wait(dev);

    uint8_t s = inb(dev->io_base + ATA_REG_STATUS);
    while (s & ATA_SR_BSY) {
        inb(dev->io_base + ATA_REG_ALTSTATUS);
        s = inb(dev->io_base + ATA_REG_ALTSTATUS);
    }

    if (advanced_check) {
        while (TRUE) {
            inb(dev->io_base + ATA_REG_ALTSTATUS);
            s = inb(dev->io_base + ATA_REG_STATUS);
            if ((s & ATA_SR_ERR) || (s & ATA_SR_DF)) {
                ata_io_wait(dev);
                uint8_t err = inb(dev->io_base + ATA_REG_ERROR);
                kloge("ATA: Device error code %d\n", err);
                halt();
            }

            if (s & ATA_SR_DRQ) {
                break;
            }

            ata_io_wait(dev);
        }
    }
}

/**
 * @brief Helper to find the max offset of an ATA device
 *
 * @param dev Device to find max offset of
 * @return uint64_t Max device offset
 */
static uint64_t ata_max_offset(ATA_DEVICE *dev) {
    uint64_t sectors = dev->identity.sectors_48;
    if (!sectors) {
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
    klogi("INIT ATA DEV %x: starting...\n", dev->io_base);

    uint16_t bus = dev->io_base;
    uint8_t slave = dev->slave;

    inb(bus + ATA_REG_STATUS);
    outb(bus + ATA_REG_ALTSTATUS, 0);

    ata_io_wait(dev);

    inb(bus + ATA_REG_STATUS);
    outb(bus + ATA_REG_HDDEVSEL, 0xA0 | slave << 4);

    ata_io_wait(dev);

    inb(bus + ATA_REG_STATUS);
    outb(bus + ATA_REG_SECCOUNT0, 0);
    inb(bus + ATA_REG_STATUS);
    outb(bus + ATA_REG_LBA0, 0);
    inb(bus + ATA_REG_STATUS);
    outb(bus + ATA_REG_LBA1, 0);
    inb(bus + ATA_REG_STATUS);
    outb(bus + ATA_REG_LBA2, 0);
    inb(bus + ATA_REG_STATUS);

    outb(bus + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

    ata_io_wait(dev);

    /* read the status port. If it is zero, the drive doesn't exist */
    uint8_t status = inb(bus + ATA_REG_STATUS);
    if (!status) {
        return 0;
    }

    int timer = 0xFFFFFF;
    while (timer--) {
        status = inb(bus + ATA_REG_STATUS);
        if (status & ATA_SR_ERR) {
            return 0;
        }
        if (!(status & ATA_SR_BSY) && status & ATA_SR_DRQ) {
            goto c1;
        }
        return 0;
    }

    c1:
        uint8_t mid = inb(bus + ATA_REG_LBA1);
        uint8_t hi = inb(bus + ATA_REG_LBA2);

        if (mid || hi) {
            /* Drive is not an ATA drive */
            return 0;
        }

        int timer2 = 0xFFFFFF;
        while (timer2--) {
            status = inb(bus + ATA_REG_STATUS);
            if (status & ATA_SR_ERR) {
                return 0;
            } else if (status & ATA_SR_DRQ) {
                goto c2;
            }
        }
        return 0;
    c2:
        status = inb(bus + ATA_REG_STATUS);
        uint16_t *buf = (uint16_t *)(&dev->identity);
        insw(bus + ATA_REG_DATA, buf, 256);
        dev->is_atapi = 0;

        uint8_t *ptr = (uint8_t *)(dev->identity.model);
        for (int i = 0; i < 39; i += 2) {
            uint8_t temp = ptr[i + 1];
            ptr[i + 1] = ptr[i];
            ptr[i] = temp;
        }

        ptr[39] = '\0';

        ptr = (uint8_t *)(dev->identity.serial);
        for (int i = 0; i < 19; i += 2) {
            uint8_t temp = ptr[i + 1];
            ptr[i + 1] = ptr[i];
            ptr[i] = temp;
        }
        ptr[19] = '\0';

        klogi("INIT ATA DEV %x: ---- Device information ----\n");
        klogi("\tDevice Name : %s\n", dev->identity.model);
        klogi("\tSerial No   : %s\n", dev->identity.serial);
        klogi("\tSectors (48): %d\n", (uint32_t)(dev->identity.sectors_48));
        klogi("\tSectors (28): %d\n", dev->identity.sectors_28);
        klogi("\tMax offset  : %d\n", ata_max_offset(dev));

        outb(dev->io_base + ATA_REG_CONTROL, 0x02);
        klogi("INIT ATA DEV %x: finished...\n", dev->io_base);
        return 1;
}

/**
 * @brief Helper to read from an ATA device
 *
 * @param dev Device to read from
 * @param lba LBA number
 * @param sector_count Number of sectors
 * @param target Target buffer to copy data into
 */
void ata_pio_read28(ATA_DEVICE *dev, uint32_t lba, uint8_t sector_count, uint8_t *target) {
    uint16_t bus = dev->io_base;
    uint8_t slave = dev->slave;

    ata_io_wait(dev);

    outb(bus + ATA_REG_HDDEVSEL, 0xE0 | slave << 4 | ((lba & 0x0F000000) >> 24));
    ata_io_wait(dev);

    outb(bus + ATA_REG_ERROR, 0x00);
    outb(bus + ATA_REG_SECCOUNT0, sector_count);
    outb(bus + ATA_REG_LBA0, (lba & 0x000000FF) >> 0);
    outb(bus + ATA_REG_LBA1, (lba & 0x0000FF00) >> 8);
    outb(bus + ATA_REG_LBA2, (lba & 0x00FF0000) >> 16);
    outb(bus + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

    for (uint64_t i = 0; i < sector_count; i++) {
        ata_poll(dev, ATA_POLL_YES);

        /* Transfer the data */
        insw(bus + ATA_REG_DATA, (void *)target, 256);
        target += 512;
        ata_io_wait(dev);
    }

    ata_poll(dev, ATA_POLL_NO);
}

/**
 * @brief Helper to write to an ATA device
 *
 * @param dev Device to read from
 * @param lba LBA number
 * @param sector_count Number of sectors
 * @param src Buffer with bytes to write
 */
void ata_pio_write28(ATA_DEVICE *dev, uint32_t lba, uint8_t sector_count, uint8_t *src) {
    uint16_t bus = dev->io_base;
    uint8_t slave = dev->slave;

    ata_io_wait(dev);

    outb(bus + ATA_REG_HDDEVSEL, 0xE0 | slave << 4 | ((lba & 0x0F000000) >> 24));
    ata_io_wait(dev);

    outb(bus + ATA_REG_ERROR, 0x00);
    outb(bus + ATA_REG_SECCOUNT0, sector_count);
    outb(bus + ATA_REG_LBA0, (lba & 0x000000FF) >> 0);
    outb(bus + ATA_REG_LBA1, (lba & 0x0000FF00) >> 8);
    outb(bus + ATA_REG_LBA2, (lba & 0x00FF0000) >> 16);
    outb(bus + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);

    ata_io_wait(dev);

    for (uint64_t i = 0; i < sector_count; i++) {
        ata_poll(dev, ATA_POLL_YES);

        /* Transfer data */
        uint16_t *data = (uint16_t *) src;
        for (size_t i = 0; i < 256; i++) {
            outw(bus + ATA_REG_DATA, data[i]);
        }

        src += 512;
        ata_io_wait(dev);
    }

    ata_poll(dev, ATA_POLL_NO);
    outb(bus + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    ata_poll(dev, ATA_POLL_NO);
}

/**
 * @brief Helper to read partition map from device
 *
 * @param dev Device to read from
 * @param dev_name Name of device
 */
STATUS ata_read_partition_map(ATA_DEVICE *dev, char *dev_name) {
    MBR mbr;
    ata_pio_read28(dev, 0, 1, (uint8_t *)&mbr);

    if (mbr.signature[0] == 0x55 && mbr.signature[1] == 0xAA) {
        klogi("ATA READ PARTITION MAP: partition table found\n");

        for (int i = 0; i < 4; i++) {
            /* FAT32 partition */
            if (mbr.partitions[i].type == 0x0B ||
                mbr.partitions[i].type == 0x0C ||
                mbr.partitions[i].type == 0x1C) {
                char part_name[2];
                char part_path[VFS_MAX_PATH_LEN];
                part_name[0] = '0' + i;
                part_name[1] = '\0';

                strcpy(part_path, "/disk/");
                strcat(part_path, part_name);

                vfs_path_to_node(part_path, CREATE, VFS_DIRECTORY);
                vfs_mount(dev_name, part_path, "fat32");
                break;
            }
        }

        klogi("ATA READ PARTITION MAP: reading partitions of %s finished...\n", dev_name);
        return SYS_OK;
    } else {
        kloge("ATA READ PARITION MAP: Did not find parition table. Signature was unknown.\n"
              "Signature was %2x %2x instead of 0x55 0xAA\n", mbr.signature[0], mbr.signature[1]);
        return SYS_ERR;
    }
}

/**
 * @brief Helper function for initializing ATAPI devices
 *
 * @param dev ATAPI device to initialize
 * @return int 0 if failure, 1 if success
 */
static int init_atapi_device(ATA_DEVICE *dev) {
    klogi("INIT ATAPI DEV %x: starting...\n", dev->io_base);
    dev->is_atapi = 1;

    outb(dev->io_base + 1, 1);
    outb(dev->control, 0);

    ata_io_wait(dev);

    uint16_t bus = dev->io_base;
    uint8_t slave = dev->slave;

    inb(bus + ATA_REG_STATUS);
    outb(bus + ATA_REG_HDDEVSEL, 0xA0 | slave << 4);
    inb(bus + ATA_REG_STATUS);
    outb(bus + ATA_REG_SECCOUNT0, 0);
    inb(bus + ATA_REG_STATUS);
    outb(bus + ATA_REG_LBA0, 0);
    inb(bus + ATA_REG_STATUS);
    outb(bus + ATA_REG_LBA1, 0);
    inb(bus + ATA_REG_STATUS);
    outb(bus + ATA_REG_LBA2, 0);
    inb(bus + ATA_REG_STATUS);

    outb(bus + ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);

    ata_io_wait(dev);

    /* Read the status port. If it's zero, the drive does not exist */
    uint8_t status = inb(bus + ATA_REG_STATUS);
    klogd("%s: waiting for status.\n", __func__);
    while (status & ATA_SR_BSY) {
        uint32_t i = 0;
        for (i = 0; i < 0x0FFFFFFF; i++) {}
        status = inb(bus + ATA_REG_STATUS);
    }

    if (!status) {
        return 0;
    }

    klogd("%s: Status indicates a drive exists. Polling...\n", __func__);
    while (status & ATA_SR_BSY) {
        status = inb(bus + ATA_REG_STATUS);
    }

    uint8_t mid = inb(bus + ATA_REG_LBA1);
    uint8_t hi = inb(bus + ATA_REG_LBA2);

    if (mid || hi) {
        return 0;
    }

    /* Wait for ERR or DRQ */
    while (!(status & (ATA_SR_ERR | ATA_SR_DRQ))) {
        status = inb(bus + ATA_REG_STATUS);
    }

    if (status & ATA_SR_ERR) {
        return 0;
    }

    /* Read from identify structure */
    uint16_t *buf = (uint16_t *)(&dev->identity);
    insw(bus + ATA_REG_DATA, buf, 256);
    dev->is_atapi = 0;

    uint8_t *ptr = (uint8_t *)(dev->identity.model);
    for (int i = 0; i < 39; i += 2) {
        uint8_t temp = ptr[i + 1];
        ptr[i + 1] = ptr[i];
        ptr[i] = temp;
    }

    ptr[39] = '\0';

    ptr = (uint8_t *)(dev->identity.serial);
    for (int i = 0; i < 19; i += 2) {
        uint8_t temp = ptr[i + 1];
        ptr[i + 1] = ptr[i];
        ptr[i] = temp;
    }
    ptr[19] = '\0';

    klogi("INIT ATAPI DEV %x: ---- Device information ----\n");
    klogi("\tDevice Name : %s\n", dev->identity.model);
    klogi("\tSerial No   : %s\n", dev->identity.serial);

    /* Detect medium */
    ATAPI_COMMAND cmd;
    cmd.command_bytes[0] = 0x25;
    cmd.command_bytes[1] = 0;
    cmd.command_bytes[2] = 0;
    cmd.command_bytes[3] = 0;
    cmd.command_bytes[4] = 0;
    cmd.command_bytes[5] = 0;
    cmd.command_bytes[6] = 0;
    cmd.command_bytes[7] = 0;
    cmd.command_bytes[8] = 0;
    cmd.command_bytes[9] = 0;
    cmd.command_bytes[10] = 0;
    cmd.command_bytes[11] = 0;

    outb(bus + ATA_REG_FEATURES, 0x00);
    outb(bus + ATA_REG_LBA1, 0x08);
    outb(bus + ATA_REG_LBA2, 0x08);
    outb(bus + ATA_REG_COMMAND, ATA_CMD_PACKET);

    /* Poll the device */
    while (TRUE) {
        uint8_t status = inb(dev->io_base + ATA_REG_STATUS);
        if (status & ATA_SR_ERR) {
            goto error;
        } else if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRDY)) {
            break;
        }
    }

    for (int i = 0; i < 6; i++) {
        outw(bus, cmd.command_words[i]);
    }

    /* Poll the device */
    while (TRUE) {
        uint8_t status = inb(dev->io_base + ATA_REG_STATUS);
        if (status & ATA_SR_ERR) {
            goto error_read;
        } else if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRDY)) {
            break;
        } else if (status & ATA_SR_DRQ) {
            break;
        }
    }

    uint16_t data[4];
    insw(bus, data, 4);
    uint32_t lba;
    uint32_t blocks;

    memcpy(&lba, &data[0], sizeof(uint32_t));
    lba = ((((lba) & 0xFF) << 24) |
          (((lba) & 0xFF00) << 8) |
          (((lba) & 0xFF0000) >> 8) |
          (((lba) & 0xFF000000) >> 24));
    memcpy(&blocks, &data[2], sizeof(uint32_t));
    blocks = ((((blocks) & 0xFF) << 24) |
             (((blocks) & 0xFF00) << 8) |
             (((blocks) & 0xFF0000) >> 8) |
             (((blocks) & 0xFF000000) >> 24));

    dev->atapi_lba = lba;
    dev->atapi_sector_size = blocks;

    if (!lba) {
        return 1;
    }

    klogi("INIT ATAPI DEV %x: LBA: %x; Block Length: %x\n", lba, blocks);
    klogi("INIT ATAPI DEV %x: finished...\n", dev->io_base);
    return 1;

    error_read:
        kloge("INIT ATAPI DEV %x: error; no medium?\n", dev->io_base);
        return 0;

    error:
        kloge("INIT ATAPI DEV %x: unsure of error\n", dev->io_base);
        return 0;
}

/**
 * @brief Helper to detect ATA device
 *
 * @param dev Device to detect what it is
 * @return 0 if unknown, 1 if parallel ATA device or emulated SATA, 2 if CDROM
 */
static int ata_device_detect(ATA_DEVICE *dev) {
    klogd("%s: going to reset device %x\n", __func__, dev);
    ata_soft_reset(dev);
    outb(dev->io_base + ATA_REG_HDDEVSEL, 0xA0 | dev->slave << 4);
    klogd("%s: Waiting on device %x\n", __func__, dev);
    ata_io_wait(dev);

    uint8_t cl = inb(dev->io_base + ATA_REG_LBA1);  /* CYL_LO */
    uint8_t ch = inb(dev->io_base + ATA_REG_LBA2);  /* CYL_HI */

    klogi("ATA DEVICE DETECT: device detected %2x %2x\n", cl, ch);
    if (cl == 0xFF && ch == 0xFF) {
        /* Nothing */
        return 0;
    } else if ((cl == 0x00 && ch == 0x00) || (cl == 0x3C && ch == 0xC3)) {
        /* Parallel ATA device, or emulated SATA */
        if (!init_ata_device(dev)) {
            klogw("ATA DEVICE DETECT: initalization of device failed!\n");
            return SYS_ERR;
        }

        char dev_name[64];
        char dev_char[2] = {ata_drive_char, '\0'};
        strcpy((char *)&dev_name, "/dev/hd");
        strcat((char *)&dev_name, (char *)&dev_char);

        VFS_TNODE *tnode = vfs_path_to_node(dev_name, CREATE, VFS_BLOCK_DEV);
        VFS_INODE *inode = vfs_alloc_inode(VFS_BLOCK_DEV, 0777, 0, NULL, tnode);
        tnode->inode = inode;
        inode->ident = (void *)dev;

        ata_read_partition_map(dev, dev_name);

        ata_drive_char++;
        return 1;
    } else if ((cl == 0x14 && ch == 0xEB) ||
               (cl == 0x69 && ch == 0x96)) {
        klogi("ATA DEVICE DETECT: Detected ATAPI device...\n");

        if (!init_atapi_device(dev)) {
            klogw("ATA DEVICE DETECT: Failed to initialize atapi device!\n");
            return 0;
        }

        char dev_name[64];
        char dev_char[2] = {(char)('1' + cdrom_number), '\0'};
        strcpy((char *)&dev_name, "/dev/cdrom");
        strcat((char *)&dev_name, (char *)&dev_char);

        VFS_TNODE *tnode = vfs_path_to_node(dev_name, CREATE, VFS_BLOCK_DEV);
        VFS_INODE *inode = vfs_alloc_inode(VFS_BLOCK_DEV, 0777, 0, NULL, tnode);
        tnode->inode = inode;
        inode->ident = (void *)dev;

        cdrom_number++;
        return 2;
    }

    /* TODO: ATAPI, SATA, SATAPI */
    return 0;
}

/**
 * @brief Main initialization function for ATA devices
 */
void init_ata() {
    ata_device_detect(&ata_primary_master);
    ata_device_detect(&ata_primary_slave);

    ata_device_detect(&ata_secondary_master);
    ata_device_detect(&ata_secondary_slave);
}