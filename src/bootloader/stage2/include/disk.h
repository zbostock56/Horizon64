#pragma once

#include <stdint.h>
#include "disk_str.h"
#include "x86.h"

uint8_t disk_initialize(DISK *, uint8_t);
uint8_t disk_read_sectors(DISK *, uint32_t, uint8_t, void *);
void disk_lba_to_chs(DISK *, uint32_t, uint16_t *, uint16_t *, uint16_t *);
