#include <dev/keyboard/keyboard.h>

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
/*
  Note: Key pressed and key releases generate different IRQs.
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

  /* TODO: Implement key combinations for special actions (e.g. CTRL + C) */
  if (key_state && ((c >= 32 && c <= 126) || (c == 8))) {
    terminal_putc(&term, c);
  }
}

// static void mouse_wait(uint8_t type) {
//   uint64_t time_out = 100000;
//   if (type == 0){
//     while (time_out--) {
//       if ((inb(PS2_STATUS_REGISTER & 1)) == 1) {

//       }
//     }
//   }
// }

void keyboard_init() {
  disable_interrupts();

  outb(PS2_COMMAND_REGISTER, COMMAND_DISABLE_FIRST_PS2_PORT);
  outb(PS2_COMMAND_REGISTER, COMMAND_DISABLE_SECOND_PS2_PORT);

  /* Flush device's buffer */
  while (inb(PS2_COMMAND_REGISTER) & 0x01) {
    inb(PS2_STATUS_REGISTER);
  }

  outb(PS2_COMMAND_REGISTER, COMMAND_ENABLE_FIRST_PS2_PORT);
  outb(PS2_COMMAND_REGISTER, COMMAND_ENABLE_SECOND_PS2_PORT);

  // uint8_t status;

  irq_register_handler(1, kbhandler);
  pic_unmask(1);
  enable_interrupts();

  klogi("Keyboard initialized...\n");
}
