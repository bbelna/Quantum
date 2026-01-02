/**
 * @file Libraries/Quantum/Include/ABI/Devices/InputDevices.hpp
 * @brief Input device syscall wrappers.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include "ABI/SystemCall.hpp"
#include "Types.hpp"

namespace Quantum::ABI::Devices {
  /**
   * Input device syscall wrappers.
   */
  class InputDevices {
    public:
      /**
       * Input device handle type.
       */
      using Handle = UInt32;

      /**
       * Input device type identifiers.
       */
      enum class Type : UInt32 {
        /**
         * Unknown or unspecified device type.
         */
        Unknown = 0,

        /**
         * Keyboard device.
         */
        Keyboard = 1,

        /**
         * Mouse device.
         */
        Mouse = 2
      };

      /**
       * Input event identifiers.
       */
      enum class EventType : UInt32 {
        /**
         * Key pressed.
         */
        KeyDown = 1,

        /**
         * Key released.
         */
        KeyUp = 2
      };

      /**
       * Input device info descriptor.
       */
      struct Info {
        /**
         * Device identifier assigned by the registry.
         */
        UInt32 id;

        /**
         * Device type identifier.
         */
        Type type;

        /**
         * Capability flags for this device.
         */
        UInt32 flags;

        /**
         * Controller-specific device index.
         */
        UInt32 deviceIndex;
      };

      /**
       * Input event descriptor.
       */
      struct Event {
        /**
         * Event type.
         */
        EventType type;

        /**
         * Source device identifier.
         */
        UInt32 deviceId;

        /**
         * Raw key code or scan code.
         */
        UInt32 keyCode;

        /**
         * Modifier key mask.
         */
        UInt32 modifiers;

        /**
         * ASCII character (0 if not available).
         */
        UInt32 ascii;

        /**
         * Unicode code point (0 if not available).
         */
        UInt32 unicode;
      };

      /**
       * Default timeout in ticks for input reads.
       */
      static constexpr UInt32 requestTimeoutTicks = 500;

      /**
       * Input device capability flags.
       */
      enum class Flag : UInt32 {
        /**
         * Device is ready for use.
         */
        Ready = 1u << 0
      };

      /**
       * Input modifier flags.
       */
      enum class Modifier : UInt32 {
        /**
         * Shift key active.
         */
        Shift = 1u << 0,

        /**
         * Control key active.
         */
        Ctrl = 1u << 1,

        /**
         * Alt key active.
         */
        Alt = 1u << 2,

        /**
         * Caps Lock active.
         */
        Caps = 1u << 3
      };

      /**
       * Input device rights.
       */
      enum class Right : UInt32 {
        /**
         * Read right.
         */
        Read = 1u << 0,

        /**
         * Control right.
         */
        Control = 1u << 1,

        /**
         * Register right.
         */
        Register = 1u << 2
      };

      /**
       * Returns the number of input devices.
       * @return
       *   Number of registered devices.
       */
      static UInt32 GetCount() {
        return InvokeSystemCall(SystemCall::Input_GetCount);
      }

      /**
       * Retrieves device info.
       * @param deviceId
       *   Identifier of the device to query.
       * @param outInfo
       *   Receives the device info.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 GetInfo(UInt32 deviceId, Info& outInfo) {
        return InvokeSystemCall(
          SystemCall::Input_GetInfo,
          deviceId,
          reinterpret_cast<UInt32>(&outInfo),
          0
        );
      }

      /**
       * Opens a handle to an input device.
       * @param deviceId
       *   Device identifier.
       * @param rights
       *   Rights mask.
       * @return
       *   Handle on success; 0 on failure.
       */
      static Handle Open(UInt32 deviceId, UInt32 rights) {
        return InvokeSystemCall(SystemCall::Input_Open, deviceId, rights, 0);
      }

      /**
       * Registers a new input device with the kernel registry.
       * @param info
       *   Device info payload (id ignored).
       * @return
       *   Assigned device id on success, 0 on failure.
       */
      static UInt32 Register(const Info& info) {
        return InvokeSystemCall(
          SystemCall::Input_Register,
          reinterpret_cast<UInt32>(&info),
          0,
          0
        );
      }

      /**
       * Updates device info for a registered device.
       * @param deviceId
       *   Device identifier to update.
       * @param info
       *   Updated info payload (id and type must match).
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 UpdateInfo(UInt32 deviceId, const Info& info) {
        return InvokeSystemCall(
          SystemCall::Input_UpdateInfo,
          deviceId,
          reinterpret_cast<UInt32>(&info),
          0
        );
      }

      /**
       * Reads the next event for a device.
       * @param deviceId
       *   Identifier of the device to read.
       * @param outEvent
       *   Receives the event.
       * @return
       *   0 on success, non-zero on failure or no event.
       */
      static UInt32 ReadEvent(UInt32 deviceId, Event& outEvent) {
        return InvokeSystemCall(
          SystemCall::Input_ReadEvent,
          deviceId,
          reinterpret_cast<UInt32>(&outEvent),
          0
        );
      }

      /**
       * Reads the next event for a device with a timeout.
       * @param deviceId
       *   Identifier of the device to read.
       * @param outEvent
       *   Receives the event.
       * @param timeoutTicks
       *   Maximum number of ticks to wait.
       * @return
       *   0 on success, non-zero on failure or timeout.
       */
      static UInt32 ReadEvent(
        UInt32 deviceId,
        Event& outEvent,
        UInt32 timeoutTicks
      ) {
        return InvokeSystemCall(
          SystemCall::Input_ReadEventTimeout,
          deviceId,
          reinterpret_cast<UInt32>(&outEvent),
          timeoutTicks
        );
      }

      /**
       * Pushes an event into the device queue.
       * @param deviceId
       *   Identifier of the device to update.
       * @param event
       *   Event payload to enqueue.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 PushEvent(UInt32 deviceId, const Event& event) {
        return InvokeSystemCall(
          SystemCall::Input_PushEvent,
          deviceId,
          reinterpret_cast<UInt32>(&event),
          0
        );
      }
  };
}
