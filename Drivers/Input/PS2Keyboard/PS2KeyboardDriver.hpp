/**
 * @file Drivers/Input/PS2Keyboard/PS2KeyboardDriver.hpp
 * @brief Declares @ref @QDrvs::Input::PS2Keyboard::PS2KeyboardDriver.
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
   * @brief PS/2 keyboard hardware driver that translates raw scancodes into
   *        input events with full modifier and extended key support.
   *
   * This class owns all hardware interaction and scancode processing. It does
   * not perform any IPC, device registration, or lifecycle management.
   */
  class PS2KeyboardDriver {
    public:
      /**
       * @brief PS/2 keyboard data port.
       */
      static constexpr UInt16 DataPort = 0x60;

      /**
       * @brief PS/2 keyboard IRQ line.
       */
      static constexpr UInt8 IRQLine = 1;

      /**
       * @brief Device ID for the PS/2 keyboard.
       */
      static constexpr DeviceID DeviceIdentifier = 101;

      /**
       * @brief Constructs the PS/2 keyboard driver.
       */
      explicit PS2KeyboardDriver() = default;

      /**
       * @brief Requests PortIO and Interrupt permissions, then claims IRQ 1.
       * @return `true` if all permissions were granted and the IRQ was
       *         claimed successfully; `false` on any failure.
       */
      bool Initialize();

      /**
       * @brief Returns the resource handle for the claimed IRQ.
       * @return The IRQ resource handle, or `static_cast<ResourceID>(-1)` if
       *         the IRQ has not been claimed.
       */
      ResourceID GetIRQHandle() const;

      /**
       * @brief Processes a raw scancode and optionally produces an input
       *        event.
       *
       * Handles the `0xE0` extended prefix (returns `false` to indicate "no
       * event yet, continue"), determines press/release, updates modifier
       * state, translates the character, and builds the `InputEvent`.
       *
       * @param rawScancode The raw byte read from the data port.
       * @param outEvent Receives the built input event when the function
       *                 returns `true`.
       * @return `true` when a complete event is ready in `outEvent`; `false`
       *         when the scancode was consumed internally (e.g. `0xE0`
       *         prefix).
       */
      bool ProcessScancode(UInt8 rawScancode, InputEvent* outEvent);

    private:
      /**
       * @brief Handle for the claimed IRQ resource.
       */
      ResourceID _irqHandle = static_cast<ResourceID>(-1);

      /**
       * @brief Whether the left Shift key is currently held.
       */
      bool _leftShiftDown = false;

      /**
       * @brief Whether the right Shift key is currently held.
       */
      bool _rightShiftDown = false;

      /**
       * @brief Whether a Ctrl key is currently held.
       */
      bool _ctrlDown = false;

      /**
       * @brief Whether an Alt key is currently held.
       */
      bool _altDown = false;

      /**
       * @brief Whether CapsLock is currently active.
       */
      bool _capsLockActive = false;

      /**
       * @brief Whether the previous scancode was the 0xE0 extended prefix.
       */
      bool _extendedPrefix = false;

      /**
       * @brief Builds the current modifier flags from tracked state.
       * @param isExtended Whether the current scancode had an `0xE0` prefix.
       * @return Combined modifier flags as a `UInt8`.
       */
      UInt8 _buildModifiers(bool isExtended);

      /**
       * @brief Updates modifier state from a scancode.
       * @param keyIndex The scancode index (bits 6-0).
       * @param isRelease Whether this is a key release.
       * @param isExtended Whether this scancode was 0xE0-prefixed.
       */
      void _updateModifierState(
        UInt8 keyIndex,
        bool isRelease,
        bool isExtended
      );

      /**
       * @brief Translates a scancode into an ASCII character and event
       *        scancode.
       * @param keyIndex The scancode index (bits 6-0).
       * @param isExtended Whether this scancode was `0xE0`-prefixed.
       * @param outScancode Receives the event scancode value.
       * @return The translated ASCII character, or `0` for non-printable keys.
       */
      char _translateCharacter(
        UInt8 keyIndex,
        bool isExtended,
        UInt8* outScancode
      );
  };
}
