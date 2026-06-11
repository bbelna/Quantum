/**
 * @file Drivers/Input/PS2Keyboard/PS2KeyboardDriver.cpp
 * @brief Implements @ref @QDrvs::Input::PS2Keyboard::PS2KeyboardDriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "PS2KeyboardDriver.hpp"
#include "PS2USKeymap.hpp"

namespace Quantum::Drivers::Input::PS2Keyboard {
  bool PS2KeyboardDriver::Initialize() {
    // request Port I/O permission (needed to read from 0x60)
    if (!Process::RequestPermission(ProcessPermissions::PortIO)) {
      KernelLog::Write(
        LogLevel::Error,
        "Failed to acquire Port I/O permission"
      );

      return false;
    }

    // request interrupt permission (needed to claim IRQ 1)
    if (!Process::RequestPermission(ProcessPermissions::Interrupts)) {
      KernelLog::Write(
        LogLevel::Error,
        "Failed to acquire interrupt permission"
      );

      return false;
    }

    // claim IRQ 1
    _irqHandle = Interrupts::Claim(IRQLine);

    if (_irqHandle == static_cast<ResourceID>(-1)) {
      KernelLog::Write(
        LogLevel::Error,
        "Failed to claim IRQ %u",
        static_cast<UInt32>(IRQLine)
      );

      return false;
    }

    KernelLog::Write(
      LogLevel::Trace,
      "Claimed IRQ %u (handle %u)",
      static_cast<UInt32>(IRQLine),
      _irqHandle
    );

    return true;
  }

  ResourceID PS2KeyboardDriver::GetIRQHandle() const {
    return _irqHandle;
  }

  bool PS2KeyboardDriver::ProcessScancode(
    UInt8 rawScancode,
    InputEvent* outEvent
  ) {
    // handle 0xE0 extended prefix
    if (rawScancode == 0xE0) {
      _extendedPrefix = true;

      return false;
    }

    // determine if this is a key press or release
    bool isRelease = (rawScancode & 0x80) != 0;
    UInt8 keyIndex = rawScancode & 0x7F;
    bool isExtended = _extendedPrefix;

    _extendedPrefix = false;

    // update modifier state
    _updateModifierState(keyIndex, isRelease, isExtended);

    // translate scancode to character
    UInt8 eventScancode;
    char character = _translateCharacter(
      keyIndex,
      isExtended,
      &eventScancode
    );

    // build the input event
    *outEvent = InputEvent {
      isRelease ? InputEventType::KeyUp : InputEventType::KeyDown,
      eventScancode,
      character,
      DeviceIdentifier,
      _buildModifiers(isExtended)
    };

    return true;
  }

  UInt8 PS2KeyboardDriver::_buildModifiers(bool isExtended) {
    UInt8 modifiers = static_cast<UInt8>(KeyModifiers::None);

    if (_leftShiftDown) {
      modifiers |= static_cast<UInt8>(KeyModifiers::LeftShift);
    }

    if (_rightShiftDown) {
      modifiers |= static_cast<UInt8>(KeyModifiers::RightShift);
    }

    if (_ctrlDown) {
      modifiers |= static_cast<UInt8>(KeyModifiers::Ctrl);
    }

    if (_altDown) {
      modifiers |= static_cast<UInt8>(KeyModifiers::Alt);
    }

    if (_capsLockActive) {
      modifiers |= static_cast<UInt8>(KeyModifiers::CapsLock);
    }

    if (isExtended) {
      modifiers |= static_cast<UInt8>(KeyModifiers::Extended);
    }

    return modifiers;
  }

  void PS2KeyboardDriver::_updateModifierState(
    UInt8 keyIndex,
    bool isRelease,
    bool isExtended
  ) {
    if (isExtended) {
      // extended modifier keys (right Ctrl, right Alt)
      if (keyIndex == 0x1D) {
        _ctrlDown = !isRelease;
      } else if (keyIndex == 0x38) {
        _altDown = !isRelease;
      }
    } else {
      // base modifier keys
      switch (keyIndex) {
        case 0x2A: {
          _leftShiftDown = !isRelease;

          break;
        } case 0x36: {
          _rightShiftDown = !isRelease;

          break;
        } case 0x1D: {
          _ctrlDown = !isRelease;

          break;
        }
        case 0x38: {
          _altDown = !isRelease;

          break;
        } case 0x3A: {
          if (!isRelease) _capsLockActive = !_capsLockActive;

          break;
        } default: {
          // not a modifier key
          break;
        }
      }
    }
  }

  char PS2KeyboardDriver::_translateCharacter(
    UInt8 keyIndex,
    bool isExtended,
    UInt8* outScancode
  ) {
    *outScancode = keyIndex;

    if (isExtended) {
      // use extended keycode map (produces KeyCode constants with bit 7 set)
      if (keyIndex < KeymapSize) {
        *outScancode = ExtendedKeyCodeMap[keyIndex];
      }

      return 0;
    }

    if (keyIndex >= KeymapSize) return 0;

    bool shiftActive = _leftShiftDown || _rightShiftDown;
    bool effectiveShift = shiftActive;

    // CapsLock affects only letters (a-z in the unshifted keymap)
    char unshiftedChar = KeymapUnshifted[keyIndex];

    if (_capsLockActive && unshiftedChar >= 'a' && unshiftedChar <= 'z') {
      effectiveShift = !effectiveShift;
    }

    return effectiveShift
      ? KeymapShifted[keyIndex]
      : KeymapUnshifted[keyIndex];
  }
}
