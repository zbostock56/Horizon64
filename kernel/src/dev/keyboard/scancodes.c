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

static char scancodes[128][2] = {
  {0, 0},     {27, 0},     {'1', '!'}, {'2', '@'}, {'3', '#'},  {'4', '$'},
  {'5', '%'}, {'6', '^'},  {'7', '&'}, {'8', '*'}, {'9', '('},  {'0', ')'},
  {'-', '_'}, {'=', '+'},  {'\b', 0},  {'\t', 0},  {'q', 'Q'},  {'w', 'W'},
  {'e', 'E'}, {'r', 'R'},  {'t', 'T'}, {'y', 'Y'}, {'u', 'U'},  {'i', 'I'},
  {'o', 'O'}, {'p', 'P'},  {'[', '{'}, {']', '}'}, {'\n', 0},   {0, 0},
  {'a', 'A'}, {'s', 'S'},  {'d', 'D'}, {'f', 'F'}, {'g', 'G'},  {'h', 'H'},
  {'j', 'J'}, {'k', 'K'},  {'l', 'L'}, {';', ':'}, {'\'', '"'}, {'`', '~'},
  {0, 0},     {'\\', '|'}, {'z', 'Z'}, {'x', 'X'}, {'c', 'C'},  {'v', 'V'},
  {'b', 'B'}, {'n', 'N'},  {'m', 'M'}, {',', '<'}, {'.', '>'},  {'/', '?'},
  {0, 0},     {'*', 0},    {0, 0},     {' ', 0},   {0, 0},      {0, 0},
  {0, 0},     {0, 0},      {0, 0},     {0, 0},     {0, 0},      {0, 0},
  {0, 0},     {0, 0},      {0, 0},     {0, 0},     {0, 0},      {0, 0},
  {0, 0},     {0, 0},      {'-', 0},   {0, 0},     {0, 0},      {0, 0},
  {'+', 0},   {0, 0},      {0, 0},     {0, 0},     {0, 0},      {0, 0},
  {0, 0},     {0, 0},      {0, 0},     {0, 0},     {0, 0},      {0, 0},
  {0, 0},     {0, 0},      {0, 0},     {0, 0},     {0, 0},      {0, 0},
  {0, 0},     {0, 0},      {0, 0},     {0, 0},     {0, 0},      {0, 0},
  {0, 0},     {0, 0},      {0, 0},     {0, 0},     {0, 0},      {0, 0},
  {0, 0},     {0, 0},      {0, 0},     {0, 0},     {0, 0},      {0, 0},
  {0, 0},     {0, 0},      {0, 0},     {0, 0},     {0, 0},      {0, 0},
  {0, 0},     {0, 0},      {0, 0},     {0, 0},     {0, 0},      {0, 0},
  {0, 0},     {0, 0}
};

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
