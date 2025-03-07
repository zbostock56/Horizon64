/**
 * @file keyboard.c
 * @author Zack Bostock
 * @brief Keyboard initialization and helpers
 * @verbatim
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <libc/stdio.h>

#include <dev/keyboard/keyboard.h>

#include <proc/callback.h>

#include <common/kprint.h>
#include <common/lock.h>

#define KB_BUFF_SIZE    (128)

static volatile KEYBOARD_MOUSE keyboard = {0};
static volatile uint8_t buffer_length = 0;
static volatile uint8_t read_index = 0;
static volatile uint8_t write_index = 0;
static volatile uint8_t key_buffer[KB_BUFF_SIZE + 1] = {0};

static LOCK kb_lock = {0};

/**
 * @brief Helper for setting a key on the keyboard
 *
 * @param state State of key
 * @param scancode Scancode to set
 */
void keyboard_set_key(uint8_t state, uint8_t scancode) {
    keyboard.key_pressed[scancode] = state;

    if (state) {
        keyboard.last_keystroke = scancode;
    } else {
        keyboard.last_keystroke = 0;
    }

    if (keyboard.ptr_to_update) {
        keyboard.ptr_to_update[scancode] = state;
    }
}

/**
 * @brief Hardware interrupt handler for the keyboard
 * @verbatim
 * Key pressed and key releases generate different IRQs.
 */
static void kbhandler() {
    uint8_t keycode = inb(PS2_DATA_PORT);
    uint8_t scancode = keycode & 0x7F;
    uint8_t key_state = !(keycode & 0x80);

    char c = get_character_from_scancode(scancode,
                                        keyboard.key_pressed[LEFT_SHIFT]
                                        | keyboard.key_pressed[RIGHT_SHIFT],
                                        keyboard.key_pressed[CAPS_LOCK]);
    keyboard_set_key(key_state, scancode);
    while (key_state && c) {
        if (keyboard.key_pressed[LEFT_CONTROL]) {
            if (buffer_length < KB_BUFF_SIZE) {
                /* Check if the D key was pressed, if so, send EOF */
                if (c == 'd' || c == 'D') {
                    LOCK_LOCK(&kb_lock);
                    key_buffer[write_index++] = EOF;
                    buffer_length++;
                    if (write_index == KB_BUFF_SIZE) {
                        write_index = 0;
                    }
                    UNLOCK_LOCK(&kb_lock);

                    cb_publish(0, CB_KEY_PRESS, EOF);
                    klogd("KBHANDLER: EOF was pressed...\n");
                    break;
                } else {
                    /* Normal keystroke */
                    LOCK_LOCK(&kb_lock);
                    key_buffer[write_index++] = c;
                    buffer_length++;
                    if (write_index == KB_BUFF_SIZE) {
                        write_index = 0;
                    }
                    UNLOCK_LOCK(&kb_lock);
                    klogd("KBHANDLER: %c was pressed\n", c);
                    cb_publish(0, CB_KEY_PRESS, c);
                    break;
                }
            }
        }
    }
}

/* TODO: Implement mouse driver */
// static void mouse_wait(uint8_t type) {
//   uint64_t time_out = 100000;
//   if (type == 0){
//     while (time_out--) {
//       if ((inb(PS2_STATUS_REGISTER & 1)) == 1) {

//       }
//     }
//   }
// }

/**
 * @brief Intialization function for the keyboard
 */
void keyboard_init() {
  klogs("INIT KEYBOARD: starting...\n");
  disable_interrupts();
  klogd("Interrupts are disabled\n");

  outb(PS2_COMMAND_REGISTER, COMMAND_DISABLE_FIRST_PS2_PORT);
  outb(PS2_COMMAND_REGISTER, COMMAND_DISABLE_SECOND_PS2_PORT);

  /* Flush device's buffer */
  //while (inb(PS2_COMMAND_REGISTER) & 0x01) {
  //  io_wait();
  //  klogd("Flushing device\n");
  //  inb(PS2_STATUS_REGISTER);
  //}

  outb(PS2_COMMAND_REGISTER, COMMAND_ENABLE_FIRST_PS2_PORT);
  outb(PS2_COMMAND_REGISTER, COMMAND_ENABLE_SECOND_PS2_PORT);

  // uint8_t status;

  klogd("Finished with outb instructions\n");
  irq_register_handler(1, kbhandler);
  klogd("Registered keyboard handler in IRQ vectors\n");
  pic_unmask(1);
  enable_interrupts();
  klogd("Enabled interrupts\n");

  klogs("INIT KEYBOARD: finished...\n");
}
