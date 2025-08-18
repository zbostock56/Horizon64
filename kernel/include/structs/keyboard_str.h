/**
 * @file keyboard_str.h
 * @author Zack Bostock
 * @brief Structs pertaining to keyboard driver functionality
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>
#include <graphics/ivec2.h>

#define MAX_SCANCODES   (128)
#define KB_BUFF_SIZE    (128)

typedef struct {
  IVEC2 mouse_offset;
  IVEC2 mouse;
  uint8_t mouse_cycle;
  uint8_t last_keystroke;
  /* 128 scancodes */
  int key_pressed[MAX_SCANCODES];
  uint8_t *ptr_to_update;
} KEYBOARD_MOUSE;

typedef struct {
    volatile uint8_t buffer[KB_BUFF_SIZE];
    volatile uint8_t read_index;
    volatile uint8_t write_index;
    volatile uint8_t buffer_length;
    volatile uint8_t overflow_occurred;
} KEYBOARD_BUFFER;
