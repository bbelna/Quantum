/**
 * @file System/Kernel/Include/Arch/IA32/PS2Keyboard.hpp
 * @brief IA32 PS/2 keyboard driver.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Interrupts.hpp>
#include <Types.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  /**
   * IA32 PS/2 (IRQ1) keyboard driver.
   */
  class PS2Keyboard {
    public:
      /**
       * Initializes the PS/2 keyboard driver.
       */
      static void Initialize();

      /**
       * Checks if a character is available in the buffer.
       * @return
       *   True if a character is available, false otherwise.
       */
      static bool KeyAvailable();

      /**
       * Reads a character from the buffer.
       * @return
       *   The character read, or 0 if none available.
       */
      static char ReadCharacter();

    private:
      /**
       * Scancode to ASCII mapping for standard keys.
       */
      static inline const char _scancodeMap[128] = {
        0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
        '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
        0,  'a','s','d','f','g','h','j','k','l',';','\'','`', 0,
        '\\','z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' ',
      };

      /**
       * Scancode to ASCII mapping when Shift is active.
       */
      inline static const char _scancodeMapShift[128] = {
        0,  27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
        '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
        0,  'a','s','d','f','g','h','j','k','l',':','\"','~', 0,
        '|','z','x','c','v','b','n','m','<','>','?', 0, '*', 0, ' ',
      };

      /**
       * Left shift make code.
       */
      static constexpr UInt8 _shiftLeftMake = 0x2A;

      /**
       * Right shift make code.
       */
      static constexpr UInt8 _shiftRightMake = 0x36;

      /**
       * Left shift break code.
       */
      static constexpr UInt8 _shiftLeftBreak = 0xAA;

      /**
       * Right shift break code.
       */
      static constexpr UInt8 _shiftRightBreak = 0xB6;

      /**
       * Control make code.
       */
      static constexpr UInt8 _ctrlMake = 0x1D;

      /**
       * Control break code.
       */
      static constexpr UInt8 _ctrlBreak = 0x9D;

      /**
       * Alt make code.
       */
      static constexpr UInt8 _altMake = 0x38;

      /**
       * Alt break code.
       */
      static constexpr UInt8 _altBreak = 0xB8;

      /**
       * Caps Lock make code.
       */
      static constexpr UInt8 _capsMake = 0x3A;

      /**
       * Caps Lock break code.
       */
      static constexpr UInt8 _capsBreak = 0xBA;

      /**
       * Keyboard input buffer size.
       */
      static constexpr Size _bufferSize = 64;

      /**
       * Keyboard input buffer.
       */
      inline static char _keyBuffer[_bufferSize] = {};

      /**
       * Head index for the keyboard buffer.
       */
      inline static volatile UInt8 _head = 0;

      /**
       * Tail index for the keyboard buffer.
       */
      inline static volatile UInt8 _tail = 0;

      /**
       * Indicates if Shift key is active.
       */
      inline static volatile bool _shiftActive = false;

      /**
       * Indicates if Caps Lock is active.
       */
      inline static volatile bool _capsLock = false;

      /**
       * Indicates if Control key is active.
       */
      inline static volatile bool _ctrlActive = false;

      /**
       * Indicates if Alt key is active.
       */
      inline static volatile bool _altActive = false;

      /**
       * Indicates if the last scancode was an extended prefix (0xE0).
       */
      inline static volatile bool _extendedPrefix = false;

      /**
       * Enqueues a character into the keyboard buffer.
       * @param ch
       *   Character to enqueue.
       */
      static void Enqueue(char ch);

      /**
       * Keyboard interrupt handler (IRQ1).
       */
      static Interrupts::Context* KeyboardHandler(
        Interrupts::Context& context
      );
  };
}
