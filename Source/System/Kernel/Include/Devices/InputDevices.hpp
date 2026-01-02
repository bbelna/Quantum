/**
 * @file System/Kernel/Include/Devices/InputDevices.hpp
 * @brief Input device registry and event queue.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

#include "Objects/Devices/InputDeviceObject.hpp"
#include "Sync/SpinLock.hpp"
#include "WaitQueue.hpp"

namespace Quantum::System::Kernel::Devices {
  /**
   * Input device registry and event queue.
   */
  class InputDevices {
    public:
      struct Device;

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
       * Input device capability flags.
       */
      enum class Flag : UInt32 {
        Ready = 1u << 0
      };

      /**
       * Event queue size per device.
       */
      static constexpr UInt32 eventQueueSize = 64;

      /**
       * Input modifier flags.
       */
      enum class Modifier : UInt32 {
        Shift = 1u << 0,
        Ctrl = 1u << 1,
        Alt = 1u << 2,
        Caps = 1u << 3
      };

      /**
       * Initializes the input device registry.
       */
      static void Initialize();

      /**
       * Registers a user-provided input device.
       * @param info
       *   Device info payload (id ignored).
       * @return
       *   Assigned device id, or 0 on failure.
       */
      static UInt32 RegisterUser(const Info& info);

      /**
       * Registers a new input device.
       * @param device
       *   Device descriptor to register.
       * @return
       *   Assigned device id, or 0 on failure.
       */
      static UInt32 Register(Device* device);

      /**
       * Unregisters an input device by id.
       * @param deviceId
       *   Identifier to remove.
       * @return
       *   True on success; false otherwise.
       */
      static bool Unregister(UInt32 deviceId);

      /**
       * Returns the number of registered input devices.
       * @return
       *   Number of devices.
       */
      static UInt32 GetCount();

      /**
       * Retrieves info for a device.
       * @param deviceId
       *   Identifier of the device to query.
       * @param outInfo
       *   Receives the device info.
       * @return
       *   True on success; false if not found.
       */
      static bool GetInfo(UInt32 deviceId, Info& outInfo);

      /**
       * Updates device info for a registered device.
       * @param deviceId
       *   Identifier of the device to update.
       * @param info
       *   Updated info payload (id and type must match).
       * @return
       *   True on success; false otherwise.
       */
      static bool UpdateInfo(UInt32 deviceId, const Info& info);

      /**
       * Reads the next event for a device.
       * @param deviceId
       *   Identifier of the device to read.
       * @param outEvent
       *   Receives the event.
       * @return
       *   True on success; false if none available.
       */
      static bool ReadEvent(UInt32 deviceId, Event& outEvent);

      /**
       * Reads the next event for a device with a timeout.
       * @param deviceId
       *   Identifier of the device to read.
       * @param outEvent
       *   Receives the event.
       * @param timeoutTicks
       *   Maximum number of ticks to wait.
       * @return
       *   True on success; false on timeout or failure.
       */
      static bool ReadEventTimeout(
        UInt32 deviceId,
        Event& outEvent,
        UInt32 timeoutTicks
      );

      /**
       * Pushes an event into the device queue.
       * @param deviceId
       *   Identifier of the device to update.
       * @param event
       *   Event payload to enqueue.
       * @return
       *   True on success; false on failure.
       */
      static bool PushEvent(UInt32 deviceId, const Event& event);

      /**
       * Retrieves the kernel object for a device.
       * @param deviceId
       *   Identifier of the device to query.
       * @return
       *   Kernel object pointer, or `nullptr` if not found.
       */
      static Objects::Devices::InputDeviceObject* GetObject(UInt32 deviceId);

      /**
       * Registered device descriptor.
       */
      struct Device {
        /**
         * Device metadata.
         */
        Info info;

        /**
         * Owner task identifier (0 for kernel devices).
         */
        UInt32 ownerId;

        /**
         * Event queue storage.
         */
        Event events[eventQueueSize];

        /**
         * Event queue head index.
         */
        UInt32 head;

        /**
         * Event queue tail index.
         */
        UInt32 tail;

        /**
         * Wait queue for readers blocking on input.
         */
        WaitQueue waitQueue;

        /**
         * Kernel object for handle-based access.
         */
        Objects::Devices::InputDeviceObject* object;
      };

    private:
      /**
       * Maximum number of registered devices.
       */
      static constexpr UInt32 _maxDevices = 8;

      /**
       * Registered device pointers.
       */
      inline static Device* _devices[_maxDevices] = { nullptr };

      /**
       * Storage for user-registered devices.
       */
      inline static Device _deviceStorage[_maxDevices] = {};

      /**
       * Number of active devices.
       */
      inline static UInt32 _deviceCount = 0;

      /**
       * Next device id to assign.
       */
      inline static UInt32 _nextDeviceId = 1;

      /**
       * Protects device registry state.
       */
      inline static Sync::SpinLock _lock;

      /**
       * Finds a device by id.
       * @param deviceId
       *   Identifier to search for.
       * @return
       *   Pointer to the device, or `nullptr` if not found.
       */
      static Device* Find(UInt32 deviceId);
  };
}
