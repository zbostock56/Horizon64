/**
 * @file scancodes.c
 * @author Zack Bostock
 * @brief Helpers for getting scancodes for the keyboard
 * @verbatim
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <dev/keyboard/scancodes.h>

/**
 * @brief Get the character from scancode object
 * @verbatim
 * Translates between the scancodes defined to the ascii typeset.
 * 
 * @param scancode Scancode to translate
 * @param shift Gets the shift code for that scancode
 * @param caps_lock Also gets the shift code (if enabled)
 * @return char Character translation
 */
char get_character_from_scancode(uint8_t scancode, uint8_t shift,
                                 uint8_t caps_lock) {
  /* Check to see if we must use the "capitals" scancodes */
  if ((caps_lock || shift) && scancodes[scancode][SHIFT_ENABLED]) {
    return scancodes[scancode][SHIFT_ENABLED];
  }

  return scancodes[scancode][SHIFT_DISABLED];
}