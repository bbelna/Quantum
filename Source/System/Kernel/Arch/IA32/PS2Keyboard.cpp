/**
 * @file System/Kernel/Arch/IA32/PS2Keyboard.cpp
 * @brief IA32 PS/2 keyboard driver. This will be removed with the
 *        implementation of KeyboardDevice.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include <Types.hpp>

#include "Arch/IA32/Interrupts.hpp"
#include "Arch/IA32/IO.hpp"
#include "Arch/IA32/PIC.hpp"
#include "Arch/IA32/PS2Keyboard.hpp"
#include "Interrupts.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  void PS2Keyboard::Enqueue(char ch) {
    UInt8 next = static_cast<UInt8>((_head + 1) % _bufferSize);

    if (next != _tail) {
      _keyBuffer[_head] = ch;
      _head = next;
    }
  }

  Interrupts::Context* PS2Keyboard::KeyboardHandler(
    Interrupts::Context& context
  ) {
    UInt8 scancode = IO::In8(0x60);

    if (scancode == 0xE0) {
      // handle E0 prefix (extended scancode); mark and skip this byte
      _extendedPrefix = true;
    } else if (_extendedPrefix) {
      // for now, ignore extended scancodes
      _extendedPrefix = false;
    } else if (scancode == _shiftLeftBreak || scancode == _shiftRightBreak) {
      // release handling
      _shiftActive = false;
    } else if (scancode == _ctrlBreak) {
      _ctrlActive = false;
    } else if (scancode == _altBreak) {
      _altActive = false;
    } else if (scancode == _capsBreak) {
      // ignore caps break
    } else if (scancode & 0x80) {
      // ignore other release events (bit 7 set)
    } else if (scancode == _shiftLeftMake || scancode == _shiftRightMake) {
      _shiftActive = true;
    } else if (scancode == _ctrlMake) {
      _ctrlActive = true;
    } else if (scancode == _altMake) {
      _altActive = true;
    } else if (scancode == _capsMake) {
      _capsLock = !_capsLock;
    } else if (scancode < sizeof(_scancodeMap)) {
      char base = _scancodeMap[scancode];
      char ch = base;

      // alphabetic keys: apply shift/caps toggling
      if (base >= 'a' && base <= 'z') {
        bool upper = (_shiftActive ^ _capsLock);
        ch = upper ? static_cast<char>(base - ('a' - 'A')) : base;
      } else if (_shiftActive) {
        ch = _scancodeMapShift[scancode];
      }

      if (ch != 0) {
        Enqueue(ch);
      }
    }

    return &context;
  }

  void PS2Keyboard::Initialize() {
    Interrupts::RegisterHandler(33, KeyboardHandler); // IRQ1 vector
    PIC::Unmask(1);
  }

  bool PS2Keyboard::KeyAvailable() {
    return _head != _tail;
  }

  char PS2Keyboard::ReadCharacter() {
    if (_head == _tail) {
      return 0;
    }
    char ch = _keyBuffer[_tail];
    _tail = static_cast<UInt8>((_tail + 1) % _bufferSize);
    return ch;
  }
}
