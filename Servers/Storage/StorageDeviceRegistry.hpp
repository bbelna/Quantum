/**
 * @file Servers/Storage/StorageDeviceRegistry.hpp
 * @brief Declares the storage device registry.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <StorageServerTypes.hpp>

#include "StorageDevice.hpp"

namespace Quantum::Servers::Storage {
  /**
   * @brief Owns and manages the set of `StorageDevice` instances known to the
   *        storage server.
   *
   * Devices are stored in a fixed-capacity array indexed by a sequential
   * numeric ID that the registry assigns on `Register`. IDs are monotonically
   * increasing and are never reused within a session.
   *
   * This class does not own the `StorageDevice` pointers it stores; the
   * caller is responsible for ensuring each device outlives its registration.
   */
  class StorageDeviceRegistry {
    public:
      /**
       * @brief Registers a newly-discovered device, assigning it a unique
       *        numeric ID.
       *
       * On success `device->_id` is set to the assigned ID. On failure (when
       * the registry is full) the device is not modified and false is
       * returned.
       *
       * @param device Pointer to the device to register. Must not be `nullptr`
       *               and must outlive its registration.
       * @return `true` if registration succeeded; `false` if the registry is
       *         full.
       */
      bool Register(StorageDevice* device);

      /**
       * @brief Removes a previously-registered device from the registry.
       *        The device pointer is set to `nullptr` in the internal array
       *        but the object is not deleted.
       * @param id The device ID returned by `Register`.
       */
      void Unregister(UInt32 id);

      /**
       * @brief Looks up a registered device by its numeric ID.
       * @param id The device ID to look up.
       * @return Pointer to the device, or `nullptr` if no device with that
       *         ID is currently registered.
       */
      StorageDevice* Find(UInt32 id) const;

      /**
       * @brief Returns the number of devices currently registered.
       * @return The device count.
       */
      UInt32 Count() const { return _count; }

      /**
       * @brief Returns the device at a zero-based position in the internal
       *        array. Useful for iterating all registered devices.
       * @param index Zero-based index, must be less than `Count()`.
       * @return Pointer to the device, or `nullptr` if the slot is empty.
       */
      StorageDevice* GetAt(UInt32 index) const;

    private:
      /**
       * @brief Internal device storage. Slots are `nullptr` until filled.
       */
      StorageDevice* _devices[ABI::MaxDevices] = {};

      /**
       * @brief Number of currently-registered devices.
       */
      UInt32 _count = 0;

      /**
       * @brief Next ID to assign on `Register`. Starts at 1 so that ID 0 can
       *        serve as an "invalid" sentinel.
       */
      UInt32 _nextID = 1;
  };
}
