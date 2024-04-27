#pragma once

typedef struct {
    int x;
    int y;
} IVEC2;

#define IVEC2_ZERO_INIT  {0, 0}
#define IVEC2_ONE_INIT   {1, 1}

#define IVEC2_ZERO       ((IVEC2) IVEC2_ZERO_INIT)
#define IVEC2_ONE        ((IVEC2) IVEC2_ONE_INIT)