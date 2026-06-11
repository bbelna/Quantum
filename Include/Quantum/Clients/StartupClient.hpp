/**
 * @file Include/Quantum/Clients/StartupClient.hpp
 * @brief Declares @ref @QClients::StartupClient.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>

namespace Quantum::Clients {
  /**
   * @brief Client-side interface to the startup server.
   *
   * Processes call @ref Ready during initialization to signal to the startup
   * server that they have completed setup and are ready to run.
   *
   * @code
   *   StartupClient startup;
   *   startup.Ready();
   * @endcode
   */
  class StartupClient {
    public:
      /**
       * @brief Creates a new @ref StartupClient instance.
       */
      StartupClient() = default;

      /**
       * @brief Signals to the startup server that this process has completed
       *        initialization and is ready to run.
       */
      void Ready();
  };
}
