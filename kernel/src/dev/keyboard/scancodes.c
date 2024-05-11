#include <dev/keyboard/scancodes.h>

/*
  Translates between the scancodes defined to the ascii typeset.
*/
char get_character_from_scancode(uint8_t scancode, uint8_t shift,
                                 uint8_t caps_lock) {
  /* Check to see if we must use the "capitals" scancodes */
  if ((caps_lock || shift) && scancodes[scancode][SHIFT_ENABLED]) {
    return scancodes[scancode][SHIFT_ENABLED];
  }

  return scancodes[scancode][SHIFT_DISABLED];
}