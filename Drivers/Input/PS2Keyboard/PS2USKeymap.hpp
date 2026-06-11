/**
 * @file Drivers/Input/PS2Keyboard/PS2USKeymap.hpp
 * @brief US keyboard scancode-to-ASCII lookup tables for PS/2 Set 1.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <PS2KeyboardDriverTypes.hpp>

namespace Quantum::Drivers::Input::PS2Keyboard {
  /**
   * @brief Number of entries in each scancode lookup table. PS/2 Set 1 uses
   *        scancodes `0x00`-`0x7F` for make codes (bit 7 clear).
   */
  inline constexpr Size KeymapSize = 128;

  /**
   * @brief US PS/2 Set 1 unshifted scancode-to-ASCII lookup table.
   *        Indexed by scancode (bits 6-0); value is corresponding ASCII
   *        character. Non-printable keys map to `0`.
   */
  inline constexpr char KeymapUnshifted[KeymapSize] = {
    0,    0,    '1',  '2',  '3',  '4',  '5',  '6',     // 0x00-0x07
    '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',    // 0x08-0x0F
    'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',     // 0x10-0x17
    'o',  'p',  '[',  ']',  '\n',  0,   'a',  's',     // 0x18-0x1F
    'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',     // 0x20-0x27
    '\'', '`',   0,  '\\',  'z',  'x',  'c',  'v',     // 0x28-0x2F
    'b',  'n',  'm',  ',',  '.',  '/',  0,    '*',     // 0x30-0x37
    0,    ' ',  0,    0,    0,    0,    0,     0,      // 0x38-0x3F
    0,    0,    0,    0,    0,    0,    0,     0,      // 0x40-0x47
    0,    0,    '-',  0,    0,    0,    '+',   0,      // 0x48-0x4F
    0,    0,    0,    0,    0,    0,    0,     0,      // 0x50-0x57
    0,    0,    0,    0,    0,    0,    0,     0,      // 0x58-0x5F
    0,    0,    0,    0,    0,    0,    0,     0,      // 0x60-0x67
    0,    0,    0,    0,    0,    0,    0,     0,      // 0x68-0x6F
    0,    0,    0,    0,    0,    0,    0,     0,      // 0x70-0x77
    0,    0,    0,    0,    0,    0,    0,     0       // 0x78-0x7F
  };

  /**
   * @brief US PS/2 Set 1 shifted scancode-to-ASCII lookup table.
   *        Same indexing as `KeymapUnshifted` but with shifted characters.
   */
  inline constexpr char KeymapShifted[KeymapSize] = {
    0,    0,    '!',  '@',  '#',  '$',  '%',  '^',   // 0x00-0x07
    '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',  // 0x08-0x0F
    'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',   // 0x10-0x17
    'O',  'P',  '{',  '}',  '\n',  0,   'A',  'S',   // 0x18-0x1F
    'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',   // 0x20-0x27
    '"',  '~',  0,    '|',  'Z',  'X',  'C',  'V',   // 0x28-0x2F
    'B',  'N',  'M',  '<',  '>',  '?',  0,    '*',   // 0x30-0x37
    0,    ' ',  0,    0,    0,    0,    0,    0,     // 0x38-0x3F
    0,    0,    0,    0,    0,    0,    0,    0,      // 0x40-0x47
    0,    0,    '-',  0,    0,    0,    '+',  0,      // 0x48-0x4F
    0,    0,    0,    0,    0,    0,    0,    0,      // 0x50-0x57
    0,    0,    0,    0,    0,    0,    0,    0,      // 0x58-0x5F
    0,    0,    0,    0,    0,    0,    0,    0,      // 0x60-0x67
    0,    0,    0,    0,    0,    0,    0,    0,      // 0x68-0x6F
    0,    0,    0,    0,    0,    0,    0,    0,      // 0x70-0x77
    0,    0,    0,    0,    0,    0,    0,    0       // 0x78-0x7F
  };

  /**
   * @brief Extended scancode (`0xE0` prefix) to KeyCode mapping.
   *        Indexed by second byte of extended scancode (`& 0x7F`);
   *        value is a `KeyCode` constant (bit 7 set), or `0` if unmapped.
   */
  inline constexpr UInt8 ExtendedKeyCodeMap[KeymapSize] = {
    0, 0, 0, 0, 0, 0, 0, 0,                                        // 0x00-0x07
    0, 0, 0, 0, 0, 0, 0, 0,                                        // 0x08-0x0F
    0, 0, 0, 0, 0, 0, 0, 0,                                        // 0x10-0x17
    0, 0, 0, 0, 0, KeyCode::RightCtrl, 0, 0,                       // 0x18-0x1F
    0, 0, 0, 0, 0, 0, 0, 0,                                        // 0x20-0x27
    0, 0, 0, 0, 0, 0, 0, 0,                                        // 0x28-0x2F
    0, 0, 0, 0, 0, 0, 0, 0,                                        // 0x30-0x37
    KeyCode::RightAlt, 0, 0, 0, 0, 0, 0, 0,                        // 0x38-0x3F
    0, 0, 0, 0, 0, 0, 0, KeyCode::Home,                            // 0x40-0x47
    KeyCode::ArrowUp, KeyCode::PageUp, 0, KeyCode::ArrowLeft, 0,   // 0x48-0x4C
    KeyCode::ArrowRight, 0, KeyCode::End,                           // 0x4D-0x4F
    KeyCode::ArrowDown, KeyCode::PageDown, KeyCode::Insert,         // 0x50-0x52
    KeyCode::Delete, 0, 0, 0, 0,                                    // 0x53-0x57
    0, 0, 0, 0, 0, 0, 0, 0,                                         // 0x58-0x5F
    0, 0, 0, 0, 0, 0, 0, 0,                                         // 0x60-0x67
    0, 0, 0, 0, 0, 0, 0, 0,                                         // 0x68-0x6F
    0, 0, 0, 0, 0, 0, 0, 0,                                         // 0x70-0x77
    0, 0, 0, 0, 0, 0, 0, 0                                          // 0x78-0x7F
  };
}
