#pragma once

#include "x86.h"
#include <boot/bootparams.h>

#define MAX_REGIONS (256)

void memory_detect(MEMORY_INFO *);