#pragma once

typedef struct {
    int x;
    int y;
    int z;
} IVEC3;

#define IVEC3_ZERO_INIT  {0, 0, 0}
#define IVEC3_ONE_INIT   {1, 1, 1}

#define IVEC3_ZERO       ((IVEC3) IVEC3_ZERO_INIT)
#define IVEC3_ONE        ((IVEC3) IVEC3_ONE_INIT)