/**
 * @file Include/Quantum/Clients/DeviceClient.hpp
 * @brief Declares @ref @QClients::DeviceClient.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>
#include <Quantum/HAL/Device.hpp>
#include <Quantum/Servers/Device/ABI.hpp>
#include <Quantum/Structures/List.hpp>

namespace Quantum::Clients {
  /**
   * @brief Client-side interface to the device server.
   *
   * Provides a clean API for querying and registering hardware devices.
   * Each method maps to a device server ABI operation via IPC.
   *
   * @code
   *   DeviceClient devices;
   *   auto list = devices.GetDevicesInCategory(
   *     ToDeviceCategoryID(DeviceCategoryType::Graphics)
   *   );
   *
   *   for (Size i = 0; i < list.GetCount(); ++i) {
   *     // inspect list[i]
   *   }
   * @endcode
   */
  class DeviceClient {
    public:
      /**
       * @brief Creates a new @ref DeviceClient instance.
       */
      DeviceClient() = default;

      /**
       * @brief Gets a list of devices in the specified category.
       * @param categoryID The ID of the device category to query.
       * @return A @ref PointerList containing the devices in the specified
       *         category. The caller owns the underlying memory and should
       *         free it when done.
       */
      Structures::Lists::PointerList<HAL::Device> GetDevicesInCategory(
        HAL::DeviceCategoryID categoryID
      );

      /**
       * @brief Adds a new device to the device server.
       * @param device The device to add. Its `CategoryID` determines which
       *        category it is placed in.
       * @return `true` if the device was added successfully, or `false` if
       *         the category is full.
       */
      bool Add(const HAL::Device& device);
  };
}
