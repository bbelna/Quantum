/**
 * @file Include/Quantum/Servers/Startup/ABI.hpp
 * @brief Declares the startup server's ABI types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>
#include <Quantum/ABI.hpp>
#include <Quantum/Types.hpp>

namespace Quantum::Servers::Startup::ABI {
  /**
   * @brief ABI version for the startup server protocol.
   */
  constexpr UInt32 StartupABIVersion = 1;

  /**
   * @brief IPC port ID for the startup server.
   */
  constexpr IPCPortID StartupPortID = 1;

  /**
   * @brief Operations supported by the startup server.
   */
  enum class StartupOperation : UInt32 {
    /**
     * @brief Tells the startup server that this process is ready.
     */
    Ready = 1
  };

  /**
   * @brief Base request type for fire-and-forget startup operations.
   */
  using StartupRequest = ABIRequest<StartupOperation>;
}
