/**
 * @file Servers/Device/DeviceServer.hpp
 * @brief Declares @ref @QDvSrv::DeviceServer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <DeviceServerTypes.hpp>

#include "DeviceController.hpp"

namespace Quantum::Servers::Device {
  /**
   * @brief Centralized system device registry and manager.
   *
   * The server registers a @ref DeviceController for device operations.
   * The base @ref Server dispatches incoming IPC messages to the
   * controller based on the operation code.
   */
  class DeviceServer : public Server {
    public:
      /**
       * @brief Creates a new @ref DeviceServer.
       * @param startupClient Reference to the @ref StartupClient for
       *                      signaling readiness. Must outlive this server
       *                      instance.
       * @param deviceController Reference to the @ref DeviceController for
       *                         device operations. Must outlive this server
       *                         instance.
       */
      DeviceServer(
        StartupClient& startupClient,
        DeviceController& deviceController
      );
  };
}
