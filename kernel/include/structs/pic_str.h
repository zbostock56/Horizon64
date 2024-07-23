/**
 * @file pic_str.h
 * @author Zack Bostock
 * @brief Structs pertaining to the Programmable Interrupt Controller driver
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>

typedef struct {
  const char *name;
  int (*probe)();
  void (*initialize)(uint8_t, uint8_t);
  void (*disable)();
  void (*send_end_of_interrupt)(int);
  void (*mask)(int);
  void (*unmask)(int);
  uint16_t (*in_request_register)();
  uint16_t (*in_service_register)();
} PIC_DRIVER;
