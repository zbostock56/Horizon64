#pragma once

#include <stdint.h>
#include <graphics/ivec2.h>

typedef struct {
  IVEC2 mouse_offset;
  IVEC2 mouse;
  uint8_t mouse_cycle;
  uint8_t last_keystroke;
  /* 128 scancodes */
  int key_pressed[128];
  uint8_t *ptr_to_update;
} KEYBOARD_MOUSE;