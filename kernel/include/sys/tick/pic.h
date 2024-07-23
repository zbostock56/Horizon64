/**
 * @file pic.h
 * @author Zack Bostock
 * @brief Information pertaining to the Programmable Interrupt Controller
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <globals.h>

#include <structs/pic_str.h>

#include <sys/asm.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
#define PIC1_COMMAND_PORT              (0x20)
#define PIC1_DATA_PORT                 (0x21)
#define PIC2_COMMAND_PORT              (0xA0)
#define PIC2_DATA_PORT                 (0xA1)

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void pic_set_mask(uint16_t mask);
void pic_disable();
void pic_mask(int irq);
void pic_unmask(int irq);
uint16_t pic_get_mask();
uint16_t pic_read_in_request_register();
uint16_t pic_read_in_service_register();
int pic_probe();
void pic_send_end_of_interrupt(int irq);
void pic_configure(uint8_t offset_pic_1, uint8_t offset_pic_2);
const PIC_DRIVER *pic_get_driver();

/* --------------------------------- DRIVER --------------------------------- */
static const PIC_DRIVER g_pic_driver = {
    .name = "8259 PIC",
    .probe = &pic_probe,
    .initialize = &pic_configure,
    .disable = &pic_disable,
    .send_end_of_interrupt = &pic_send_end_of_interrupt,
    .mask = &pic_mask,
    .unmask = &pic_unmask,
    .in_request_register = &pic_read_in_request_register,
    .in_service_register = &pic_read_in_service_register,
};
