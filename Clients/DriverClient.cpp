/**
 * @file Clients/DriverClient.cpp
 * @brief Implements @ref @QClients::DriverClient.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "DriverClient.hpp"

namespace Quantum::Clients {
  bool DriverClient::Initialize(UInt32 deviceID) {
    _deviceID = deviceID;

    return _deviceID != 0;
  }

  UInt32 DriverClient::InvokeDriver(UInt32 operation, void* payload) {
    return _kernel.InvokeDriver(_deviceID, operation, payload);
  }
}
