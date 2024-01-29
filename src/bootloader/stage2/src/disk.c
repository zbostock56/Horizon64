#include "../include/disk.h"

uint8_t disk_initialize(DISK *disk, uint8_t drive_number) {
  uint8_t drive_type = 0;
  uint16_t cylinders = 0;
  uint16_t sectors = 0;
  uint16_t heads = 0;
  if (!x86_disk_get_drive_params(drive_number, &drive_type, &cylinders, &sectors, &heads)) {
    return 0;
  }

  disk->id = drive_number;
  disk->cylinders = cylinders;
  disk->sectors = sectors;
  disk->heads = heads;
  return 1;
}

void disk_lba_to_chs(DISK *disk, uint32_t lba, uint16_t *cylinder,
                     uint16_t *head, uint16_t *sector) {
  /*
    sector = (LBA % sectors_per_track + 1)
    cylinder = (LBA / sectors_per_track) / number_of_heads
    head = (LBA / number_of_sectors) % number_of_heads
  */
  *sector = lba % disk->sectors + 1;
  *cylinder = (lba / disk->sectors) / disk->heads;
  *head = (lba / disk->sectors) % disk->heads;
}

uint8_t disk_read_sectors(DISK *disk, uint32_t lba, uint8_t sectors,
                          void *data_out) {
  uint16_t cylinder = 0;
  uint16_t sector = 0;
  uint16_t head = 0;

  disk_lba_to_chs(disk, lba, &cylinder, &head, &sector);

  for (int i = 0; i < 3; i++) {
    if (x86_disk_read(disk->id, cylinder, sector, head, sectors, data_out)) {
      return 1;
    }
    x86_disk_reset(disk->id);
  }
  return 0;
}
