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

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
#define PIC1_COMMAND_PORT              (0x20)
#define PIC1_DATA_PORT                 (0x21)
#define PIC2_COMMAND_PORT              (0xA0)
#define PIC2_DATA_PORT                 (0xA1)

/* ----------------------------- STATIC GLOBALS ----------------------------- */
static uint16_t g_pic_mask = 0xFFFF;
static int g_auto_eoi = 0;

enum {
    PIC_ICW1_ICW4           = 0x01,     /* If set, ICW4 must be read (its optional) */
    PIC_ICW1_SINGLE         = 0x02,     /* If set, means there is only one 8259 PIC */
    PIC_ICW1_INTERVAL4      = 0x04,     /* Address interval */
    PIC_ICW1_LEVEL          = 0x08,     /* Edge or level triggered mode */
    PIC_ICW1_INITIALIZE     = 0x10,     /* Tell PIC we are initializing (required!) */
} PIC_ICW1;

enum {
    PIC_ICW4_8086           = 0x01,      /* Set for 8086 platform */
    PIC_ICW4_AUTO_EOI       = 0x02,      /* Automatically set the end of interrupt flag */
    PIC_ICW4_BUFFER_MASTER  = 0x04,      /* Do we buffer the master PIC? */
    PIC_ICW4_BUFFER_SLAVE   = 0x00,      /* Do we buffer the slave PIC? */
    PIC_ICW4_BUFFERRED      = 0x08,      /* Do we buffer interrupts as they come in? */
    PIC_ICW4_SFNM           = 0x10,      /* Specially fully nested mode */
} PIC_ICW4;

enum {
    PIC_CMD_END_OF_INTERRUPT    = 0x20,
    PIC_CMD_READ_IRR            = 0x0A,
    PIC_CMD_READ_ISR            = 0x0B,
} PIC_CMD;

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

/* --------------------------- EXTERNALLY DEFINED --------------------------- */

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
