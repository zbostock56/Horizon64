#include "i8259.h"
#include "pic.h"
#include <arch/i686/general.h>

#define PIC1_COMMAND_PORT              (0x20)
#define PIC1_DATA_PORT                 (0x21)
#define PIC2_COMMAND_PORT              (0xA0)
#define PIC2_DATA_PORT                 (0xA1)

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
    PIC_ICW4_SFNM           = 0x10,     /* Specially fully nested mode */
} PIC_ICW4;

enum {
    PIC_CMD_END_OF_INTERRUPT    = 0x20,
    PIC_CMD_READ_IRR            = 0x0A,
    PIC_CMD_READ_ISR            = 0x0B,
} PIC_CMD;

static uint16_t g_pic_mask = 0xFFFF;
static int g_auto_eoi = 0;

void i8259_set_mask(uint16_t mask) {
    g_pic_mask = mask;
    i686_outb(PIC1_DATA_PORT, g_pic_mask & 0xFF);
    i686_io_wait();
    i686_outb(PIC2_DATA_PORT, g_pic_mask >> 8);
    i686_io_wait();
}

void i8259_disable() {
    i8259_set_mask(0xFFFF);
}

void i8259_mask(int irq) {
    i8259_set_mask(g_pic_mask | (1 << irq));
}

void i8259_unmask(int irq) {
    i8259_set_mask(g_pic_mask & ~(1 << irq));
}

uint16_t i8259_get_mask() {
    return i686_inb(PIC1_DATA_PORT) | (i686_inb(PIC2_DATA_PORT) << 8);
}

uint16_t i8259_read_in_request_register() {
    i686_outb(PIC1_COMMAND_PORT, PIC_CMD_READ_IRR);
    i686_outb(PIC2_COMMAND_PORT, PIC_CMD_READ_IRR);
    return ((uint16_t) i686_inb(PIC2_COMMAND_PORT)) | (((uint16_t) i686_inb(PIC2_COMMAND_PORT)) << 8);
}

uint16_t i8259_read_in_service_register() {
    i686_outb(PIC1_COMMAND_PORT, PIC_CMD_READ_ISR);
    i686_outb(PIC2_COMMAND_PORT, PIC_CMD_READ_ISR);
    return ((uint16_t) i686_inb(PIC2_COMMAND_PORT)) | (((uint16_t) i686_inb(PIC2_COMMAND_PORT)) << 8);
}

int i8259_probe() {
    i8259_disable();
    i8259_set_mask(0x0);
    return (i8259_get_mask() == 0x0);
}

void i8259_send_end_of_interrupt(int irq) {
    if (irq >= 8) {
        i686_outb(PIC2_COMMAND_PORT, PIC_CMD_END_OF_INTERRUPT);
    }
    i686_outb(PIC1_COMMAND_PORT, PIC_CMD_END_OF_INTERRUPT);
}

void i8259_PIC_configure(uint8_t offset_pic_1, uint8_t offset_pic_2) {
    /* Mask all interrupts */
    i8259_set_mask(0xFFFF);

    /* Initialize control word 1 */
    i686_outb(PIC1_COMMAND_PORT, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);
    i686_io_wait();
    i686_outb(PIC2_COMMAND_PORT, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);
    i686_io_wait();

    /* Initialize control word 2 - the offsets */
    i686_outb(PIC1_DATA_PORT, offset_pic_1);
    i686_io_wait();
    i686_outb(PIC2_DATA_PORT, offset_pic_2);
    i686_io_wait();

    /* Initialize control word 3 */
    /* Tell PIC1 that the slave PIC is at IRQ2 (b0100 || 0x4) */
    i686_outb(PIC1_DATA_PORT, 0x4);
    i686_io_wait();
    /* Tell PIC2 that its cascade identity is (b0010 || 0x2) */
    i686_outb(PIC2_DATA_PORT, 0x2);
    i686_io_wait();

    /* Initialize control word 4 */
    i686_outb(PIC1_DATA_PORT, PIC_ICW4_8086);
    i686_io_wait();
    i686_outb(PIC2_DATA_PORT, PIC_ICW4_8086);
    i686_io_wait();

    /* Mask all interrupts until they are enabled by device driver */
    i8259_set_mask(0xFFFF);
}

static const PICDRIVER g_pic_driver = {
    .name = "8259 PIC",
    .probe = &i8259_probe,
    .initialize = &i8259_PIC_configure,
    .disable = &i8259_disable,
    .send_end_of_interrupt = &i8259_send_end_of_interrupt,
    .mask = &i8259_mask,
    .unmask = &i8259_unmask,
};

const PICDRIVER *i8259_get_driver() {
    return &g_pic_driver;
}