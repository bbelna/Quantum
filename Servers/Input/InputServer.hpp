/**
 * @file Servers/Input/InputServer.hpp
 * @brief Declares @ref @QInSrv::InputServer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <InputServerTypes.hpp>

#include "InputEventController.hpp"

namespace Quantum::Servers::Input {
  /**
   * @brief The input server responsible for buffering and dispatching
   *        input events between device drivers and user-space clients.
   *
   * The server registers an @ref InputEventController for all input
   * operations. The base @ref Server dispatches incoming IPC messages
   * to the controller based on the operation code.
   */
  class InputServer : public Server {
    public:
      /**
       * @brief Creates a new @ref InputServer.
       * @param startupClient Reference to the @ref StartupClient for
       *                      signaling readiness. Must outlive this server
       *                      instance.
       * @param inputEventController Reference to the
       *                             @ref InputEventController for input
       *                             event operations. Must outlive this
       *                             server instance.
       */
      InputServer(
        StartupClient& startupClient,
        InputEventController& inputEventController
      );
  };
}
