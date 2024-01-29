#pragma once
#include "../include/mbr.h"

#define max(a,b) \
    ({__typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
      _a > _b ? _a : b; \
    })

#define min(a,b) \
    ({__typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
      _a < _b ? _a : b; \
    })
    
void print_partition_disk(PARTITION *);
void hang_system(const char *);