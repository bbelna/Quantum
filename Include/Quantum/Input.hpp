/**
 * @file Include/Quantum/Input.hpp
 * @brief Declaration of shared input event types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Enum.hpp>
#include <Quantum/HAL/Device.hpp>
#include <Quantum/Core/Types.hpp>

/**
 * @brief Quantum's input library.
 */
namespace Quantum::Input {
  /**
   * @brief Type of an input event.
   */
  enum class InputEventType : UInt8 {
    /**
     * @brief A key was pressed down.
     */
    KeyDown = 1,

    /**
     * @brief A key was released.
     */
    KeyUp = 2,

    /**
     * @brief The mouse was moved.
     */
    MouseMove = 3,

    /**
     * @brief A mouse button was pressed.
     */
    MouseButtonDown = 4,

    /**
     * @brief A mouse button was released.
     */
    MouseButtonUp = 5,

    /**
     * @brief The mouse scroll wheel was moved.
     */
    MouseScroll = 6
  };

  /**
   * @brief Modifier key flags carried with each input event.
   */
  enum class KeyModifiers : UInt8 {
    /**
     * @brief No modifier keys are active.
     */
    None = 0,

    /**
     * @brief The left Shift key is held.
     */
    LeftShift = 1 << 0,

    /**
     * @brief The right Shift key is held.
     */
    RightShift = 1 << 1,

    /**
     * @brief Either Ctrl key is held.
     */
    Ctrl = 1 << 2,

    /**
     * @brief Either Alt key is held.
     */
    Alt = 1 << 3,

    /**
     * @brief Caps Lock is currently active.
     */
    CapsLock = 1 << 4,

    /**
     * @brief The scancode that triggered this event has the extended-key
     *        prefix (`0xE0`), indicating a key from the extended key set.
     */
    Extended = 1 << 5
  };

}

QUANTUM_ENABLE_ENUM_BITMASK_OPS(Quantum::Input, KeyModifiers)

namespace Quantum::Input {
  /**
   * @brief Mouse button flags.
   */
  enum class MouseButton : UInt8 {
    /**
     * @brief The primary (left) mouse button.
     */
    Left = 1 << 0,

    /**
     * @brief The secondary (right) mouse button.
     */
    Right = 1 << 1,

    /**
     * @brief The middle (scroll wheel) mouse button.
     */
    Middle = 1 << 2,
  };
}

QUANTUM_ENABLE_ENUM_BITMASK_OPS(Quantum::Input, MouseButton)

namespace Quantum::Input {
  /**
   * @brief Virtual key codes for non-printable and extended keys.
   */
  namespace KeyCode {
    /**
     * @brief No key / unrecognised scancode.
     */
    inline constexpr UInt8 None = 0x00;

    /**
     * @brief Escape key.
     */
    inline constexpr UInt8 Escape = 0x01;

    /**
     * @brief Backspace key.
     */
    inline constexpr UInt8 Backspace = 0x0E;

    /**
     * @brief Tab key.
     */
    inline constexpr UInt8 Tab = 0x0F;

    /**
     * @brief Enter (Return) key.
     */
    inline constexpr UInt8 Enter = 0x1C;

    /**
     * @brief Left Ctrl key.
     */
    inline constexpr UInt8 LeftCtrl = 0x1D;

    /**
     * @brief Left Shift key.
     */
    inline constexpr UInt8 LeftShift = 0x2A;

    /**
     * @brief Right Shift key.
     */
    inline constexpr UInt8 RightShift = 0x36;

    /**
     * @brief Left Alt key.
     */
    inline constexpr UInt8 LeftAlt = 0x38;

    /**
     * @brief Caps Lock key.
     */
    inline constexpr UInt8 CapsLock = 0x3A;

    /**
     * @brief F1 function key.
     */
    inline constexpr UInt8 F1 = 0x3B;

    /**
     * @brief F2 function key.
     */
    inline constexpr UInt8 F2 = 0x3C;

    /**
     * @brief F3 function key.
     */
    inline constexpr UInt8 F3 = 0x3D;

    /**
     * @brief F4 function key.
     */
    inline constexpr UInt8 F4 = 0x3E;

    /**
     * @brief F5 function key.
     */
    inline constexpr UInt8 F5 = 0x3F;

    /**
     * @brief F6 function key.
     */
    inline constexpr UInt8 F6 = 0x40;

    /**
     * @brief F7 function key.
     */
    inline constexpr UInt8 F7 = 0x41;

    /**
     * @brief F8 function key.
     */
    inline constexpr UInt8 F8 = 0x42;

    /**
     * @brief F9 function key.
     */
    inline constexpr UInt8 F9 = 0x43;

    /**
     * @brief F10 function key.
     */
    inline constexpr UInt8 F10 = 0x44;

    /**
     * @brief Num Lock key.
     */
    inline constexpr UInt8 NumLock = 0x45;

    /**
     * @brief Scroll Lock key.
     */
    inline constexpr UInt8 ScrollLock = 0x46;

    /**
     * @brief F11 function key.
     */
    inline constexpr UInt8 F11 = 0x57;

    /**
     * @brief F12 function key.
     */
    inline constexpr UInt8 F12 = 0x58;

    /**
     * @brief Bit mask applied to extended-key scancodes (`0xE0`-prefixed).
     *        When set in a scancode value, the key originates from the
     *        extended key set rather than the base AT set.
     */
    inline constexpr UInt8 ExtendedFlag = 0x80;

    /**
     * @brief Up arrow key (extended).
     */
    inline constexpr UInt8 ArrowUp = ExtendedFlag | 0x48;

    /**
     * @brief Down arrow key (extended).
     */
    inline constexpr UInt8 ArrowDown = ExtendedFlag | 0x50;

    /**
     * @brief Left arrow key (extended).
     */
    inline constexpr UInt8 ArrowLeft = ExtendedFlag | 0x4B;

    /**
     * @brief Right arrow key (extended).
     */
    inline constexpr UInt8 ArrowRight = ExtendedFlag | 0x4D;

    /**
     * @brief Home key (extended).
     */
    inline constexpr UInt8 Home = ExtendedFlag | 0x47;

    /**
     * @brief End key (extended).
     */
    inline constexpr UInt8 End = ExtendedFlag | 0x4F;

    /**
     * @brief Page Up key (extended).
     */
    inline constexpr UInt8 PageUp = ExtendedFlag | 0x49;

    /**
     * @brief Page Down key (extended).
     */
    inline constexpr UInt8 PageDown = ExtendedFlag | 0x51;

    /**
     * @brief Insert key (extended).
     */
    inline constexpr UInt8 Insert = ExtendedFlag | 0x52;

    /**
     * @brief Delete key (extended).
     */
    inline constexpr UInt8 Delete = ExtendedFlag | 0x53;

    /**
     * @brief Right Ctrl key (extended).
     */
    inline constexpr UInt8 RightCtrl = ExtendedFlag | 0x1D;

    /**
     * @brief Right Alt (AltGr) key (extended).
     */
    inline constexpr UInt8 RightAlt = ExtendedFlag | 0x38;
  }

  /**
   * @brief Describes a single input event from an input device.
   */
  struct InputEvent {
    /**
     * @brief The type of input event (key down or key up).
     */
    InputEventType Type;

    /**
     * @brief The raw hardware scancode, or a `KeyCode` constant for extended
     *        keys (bit 7 set).
     */
    UInt8 Scancode;

    /**
     * @brief The translated ASCII character, or `0` if the key is
     *        non-printable.
     */
    char Character;

    /**
     * @brief The ID of the device that generated this event.
     */
    HAL::DeviceID SourceDeviceID;

    /**
     * @brief Active modifier flags at the time of this event.
     */
    UInt8 Modifiers;

    /**
     * @brief Relative mouse movement along the X axis.
     */
    Int16 DeltaX;

    /**
     * @brief Relative mouse movement along the Y axis.
     */
    Int16 DeltaY;

    /**
     * @brief Mouse button state.
     */
    MouseButton Buttons;

    /**
     * @brief Relative mouse scroll wheel movement along the Z axis.
     *        Positive = scroll up, negative = scroll down.
     */
    Int8 DeltaZ;
  };
}
