/**
 * @file Include/Quantum/Clients/InputClient.hpp
 * @brief Declares @ref @QClients::InputClient.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>
#include <Quantum/Input.hpp>
#include <Quantum/Kernel/IPC/IPCTypes.hpp>

#include "KernelClient.hpp"

namespace Quantum::Clients {
  /**
   * @brief Client-side interface to the input server.
   *
   * Provides blocking and non-blocking keyboard/mouse event retrieval.
   * Caches IPC handles for the lifetime of the client to avoid per-call
   * port open/close overhead.
   *
   * @code
   *   InputClient input;
   *   if (input.Initialize()) {
   *     Input::InputEvent event;
   *     if (input.GetNextEvent(&event)) { ... }
   *   }
   * @endcode
   */
  class InputClient {
    public:
      /**
       * @brief Creates a new @ref InputClient instance.
       */
      InputClient() = default;

      /**
       * @brief Destroys the @ref InputClient, closing cached IPC handles.
       */
      ~InputClient();

      /**
       * @brief Opens IPC handles to the input server.
       * @return `true` if handles were opened successfully.
       */
      bool Initialize();

      /**
       * @brief Blocks until the next input event is available.
       * @param outEvent Pointer to receive the event.
       * @return `true` if an event was received.
       */
      bool GetNextEvent(Input::InputEvent* outEvent);

      /**
       * @brief Returns immediately with the next event, or `false` if none.
       * @param outEvent Pointer to receive the event.
       * @return `true` if an event was received.
       */
      bool TryGetNextEvent(Input::InputEvent* outEvent);

    private:
      /**
       * @brief Kernel client for IPC and memory operations.
       */
      KernelClient _kernel;

      /**
       * @brief Cached send handle to the input server.
       */
      Kernel::IPC::IPCPortResourceID _sendHandle = static_cast<Kernel::IPC::IPCPortResourceID>(-1);

      /**
       * @brief Cached receive handle for reply messages.
       */
      Kernel::IPC::IPCPortResourceID _replyHandle = static_cast<Kernel::IPC::IPCPortResourceID>(-1);

      /**
       * @brief Auto-assigned reply port ID.
       */
      Kernel::IPC::IPCPortID _replyPortID = 0;

      /**
       * @brief Whether the client has been initialized.
       */
      bool _initialized = false;
  };
}
