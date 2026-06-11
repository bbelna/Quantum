/**
 * @file Include/Quantum/Clients/DriverClient.hpp
 * @brief Declares @ref @QClients::DriverClient.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>

#include "KernelClient.hpp"

namespace Quantum::Clients {
  /**
   * @brief Base class for driver clients that dispatch operations to a
   *        platform device driver via @ref KernelClient::InvokeDriver.
   *
   * Provides the universal mechanism for invoking driver operations
   * regardless of whether the driver lives in kernel space or runs as
   * a userspace process. Derived classes add typed wrapper methods for
   * their specific driver's operations.
   *
   * @code
   *   class MyDriverClient : public DriverClient {
   *     public:
   *       void DoSomething() {
   *         MyPayload payload { ... };
   *         InvokeDriver(Enum::ToBase(MyOperation::DoSomething), &payload);
   *       }
   *   };
   * @endcode
   */
  class DriverClient {
    public:
      /**
       * @brief Creates a new @ref DriverClient with no device bound.
       */
      DriverClient() = default;

      /**
       * @brief Virtual destructor for safe polymorphic destruction.
       */
      virtual ~DriverClient() = default;

      /**
       * @brief Binds this client to a previously discovered device ID.
       * @param deviceID The device ID obtained from the device server.
       * @return `true` if the device ID is valid (non-zero).
       */
      bool Initialize(UInt32 deviceID);

      /**
       * @brief Returns the bound device ID.
       * @return The device ID, or 0 if not initialized.
       */
      UInt32 GetDeviceID() const { return _deviceID; }

    protected:
      /**
       * @brief Dispatches an operation to the bound driver.
       * @param operation The driver-specific operation code.
       * @param payload Pointer to the operation-specific payload structure.
       * @return The result of the driver operation.
       */
      UInt32 InvokeDriver(UInt32 operation, void* payload);

      /**
       * @brief Kernel client for driver dispatch.
       */
      KernelClient _kernel;

      /**
       * @brief Cached device ID for driver dispatch.
       */
      UInt32 _deviceID = 0;
  };
}
