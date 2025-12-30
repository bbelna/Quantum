/**
 * @file Services/Drivers/Input/PS2/Keyboard/Include/Driver.hpp
 * @brief PS/2 keyboard driver.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <ABI/Devices/InputDevices.hpp>
#include <ABI/IPC.hpp>
#include <ABI/IRQ.hpp>
#include <Types.hpp>

namespace Quantum::Services::Drivers::Input::PS2::Keyboard {
  /**
   * PS/2 keyboard driver entry point.
   */
  class Driver {
    public:
      /**
       * Driver main entry point.
       */
      static void Main();

    private:
      /**
       * Keyboard IRQ line.
       */
      static constexpr UInt32 _irqLine = 1;

      /**
       * Input device identifier assigned by the kernel.
       */
      inline static UInt32 _deviceId = 0;

      /**
       * Input device handle for event submissions.
       */
      inline static ABI::Devices::InputDevices::Handle _deviceHandle = 0;

      /**
       * IPC port identifier for IRQ delivery.
       */
      inline static UInt32 _portId = 0;

      /**
       * IRQ handle granted by the coordinator.
       */
      inline static ABI::IRQ::Handle _irqHandle = 0;

      /**
       * Indicates if Shift key is active.
       */
      inline static bool _shiftActive = false;

      /**
       * Indicates if Caps Lock is active.
       */
      inline static bool _capsLock = false;

      /**
       * Indicates if Control key is active.
       */
      inline static bool _ctrlActive = false;

      /**
       * Indicates if Alt key is active.
       */
      inline static bool _altActive = false;

      /**
       * Indicates if the last scancode was an extended prefix (0xE0).
       */
      inline static bool _extendedPrefix = false;

      /**
       * Registers the IRQ route with the coordinator.
       * @param portId
       *   IPC port identifier to receive IRQ messages.
       */
      static void RegisterIRQRoute(UInt32 portId);

      /**
       * Sends a readiness message to the coordinator.
       * @param deviceTypeId
       *   Device type identifier.
       */
      static void SendReadySignal(UInt8 deviceTypeId);

      /**
       * Checks whether an IPC message is an IRQ notification.
       * @param msg
       *   IPC message to check.
       * @return
       *   True if the message is an IRQ notification.
       */
      static bool IsIRQMessage(const ABI::IPC::Message& msg);

      /**
       * Handles an IRQ notification by reading a scancode.
       */
      static void HandleIRQ();

      /**
       * Handles a scancode and emits input events.
       * @param scancode
       *   Scancode to process.
       */
      static void HandleScancode(UInt8 scancode);

      /**
       * Builds the current modifier mask.
       * @return
       *   Modifier mask.
       */
      static UInt32 BuildModifiers();

      /**
       * Sends a key event to the kernel input registry.
       * @param keyCode
       *   Raw key code or scan code.
       * @param type
       *   Event type (key down or key up).
       * @param ascii
       *   ASCII character (0 if not available).
       * @param unicode
       *   Unicode code point (0 if not available).
       */
      static void SendKeyEvent(
        UInt32 keyCode,
        ABI::Devices::InputDevices::EventType type,
        UInt32 ascii,
        UInt32 unicode
      );
  };
}
