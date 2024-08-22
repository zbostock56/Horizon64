/**
 * @file serial.c
 * @author Zack Bostock
 * @brief Functionality pertaining to serial port operation
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <dev/serial.h>

static ERRNO serial_enabled = ERR_NO_ERR;

/**
 * @brief Initialization function for serial port
 * @verbatim
 * Steps:
 * 1. Disable all interrupts
 * 2. Enable DLAB (set baud rate divisor)
 * 3. Set baud rate using divisor from (115200 / rate)
 * 4. Command 8 bits, no parity, one stop bit
 * 5. Enable FIFO, clear them, 14-byte threshold
 * 6. IRQs enabled, RTS/DSR set
 * 7. Set in loopback mode, test the serial chip
 * 8. Check if faulty, if not, set in normal operation mode
 *
 * @return STATUS SYS_OK if normal, SYS_ERR if faulty
 */
STATUS serial_init() {
    klogi("INIT SERIAL: starting...\n");
    /* Step 1 */
    outb(COM1 + 1, 0x00);
    /* Step 2 */
    outb(COM1 + 3, 0x80);
    /* Step 3 */
    uint16_t divisor = (uint16_t) (MAX_RATE / BAUD_RATE);
    /* Low byte */
    outb(COM1 + 0, (divisor >> 0) & 0xFF);
    /* High byte */
    outb(COM1 + 1, (divisor >> 8) & 0xFF);
    /* Step 4 */
    outb(COM1 + 3, 0x03);
    /* Step 5 */
    outb(COM1 + 2, 0xC7);
    /* Step 6 */
    outb(COM1 + 4, 0x0B);
    /* Step 7 */
    outb(COM1 + 4, 0xAE);
    /* Step 8 */
    outb(COM1 + 0, 0xAE);
    if (inb(COM1 + 0) != 0xAE) {
        serial_enabled = ERR_SERIAL_FAULTY;
        klogi("INIT SERIAL: finished...\n");
        return SYS_ERR;
    }
    /* Set in normal operation mode */
    outb(COM1 + 4, 0x0F);
    return SYS_OK;
    klogi("INIT SERIAL: finished...\n");
}

/**
 * @brief Reads in the value from the input buffer on COM1.
 *
 * @return uint8_t Value from the input buffer
 */
uint8_t serial_recieved() {
    return (inb(COM1 + 5) & 0x01);
}

/**
 * @brief Reads in the value from the output buffer on COM1.
 *
 * @return uint8_t Value from the output buffer
 */
uint8_t is_transmit_empty() {
    return (inb(COM1 + 5) & 0x20);
}

/**
 * @brief Writes a character to COM1.
 *
 * @param c Character to write
 * @return STATUS SYS_ERR if error, SYS_OK if successful
 */
STATUS serial_write(char c) {
    if (serial_enabled == ERR_SERIAL_FAULTY) {
        return SYS_ERR;
    }

    /* Wait until communication can be sent*/
    while(!is_transmit_empty());

    outb(COM1, c);
    return SYS_OK;
}

/**
 * @brief Reads a character from COM1.
 *
 * @return char Read in character from COM1
 */
char serial_read() {
    while (!serial_recieved());
    return inb(COM1);
}

/**
 * @brief Writes a string to COM1.
 *
 * @param str String to write
 * @return STATUS SYS_OK if success, SYS_ERR if failed
 */
STATUS serial_puts(const char *str) {
  for (size_t i = 0; i < __builtin_strlen(str); i++) {
    if (serial_write(str[i]) == SYS_ERR) {
        return SYS_ERR;
    }
  }
  return SYS_OK;
}
