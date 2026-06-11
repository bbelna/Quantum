/**
 * @file Include/Quantum/Servers/Device/ABI.hpp
 * @brief Declares the device server's ABI types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>
#include <Quantum/HAL/Device.hpp>
#include <Quantum/ABI.hpp>

namespace Quantum::Servers::Device::ABI {
  using HAL::Device;
  using HAL::DeviceCategoryID;

  /**
   * @brief ABI version for the device server protocol.
   */
  constexpr UInt32 DeviceABIVersion = 1;

  /**
   * @brief IPC port ID for the device server.
   */
  constexpr Kernel::IPC::IPCPortID DevicePortID = 3;

  /**
   * @brief Operations supported by the device server.
   */
  enum class DeviceOperation : UInt32 {
    /**
     * @brief Get a list of devices in a specified category.
     */
    GetDevicesInCategory = 1,

    /**
     * @brief Add a device to the device tree.
     */
    AddDevice = 2,
  };

  /**
   * @brief Base request type for fire-and-forget device operations.
   */
  using DeviceRequest = ABIRequest<DeviceOperation>;

  /**
   * @brief Base request type for device operations that expect a reply.
   */
  using DeviceRequestWithReply
    = ABIRequestWithReplyPort<DeviceOperation>;

  /**
   * @brief Request structure for the `GetDevicesInCategory` operation.
   */
  struct DeviceGetDevicesInCategoryRequest
    : public DeviceRequestWithReply
  {
    /**
     * @brief The ID of the device category to query.
     */
    DeviceCategoryID CategoryID;
  };

  /**
   * @brief Request structure for the `AddDevice` operation.
   */
  struct DeviceAddRequest : public DeviceRequestWithReply {
    /**
     * @brief The device to add.
     */
    Device DeviceToAdd;
  };
}
