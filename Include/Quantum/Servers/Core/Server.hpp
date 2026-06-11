/**
 * @file Include/Quantum/Servers/Core/Server.hpp
 * @brief Declares @ref @QSrvCore::Server.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>
#include <Quantum/Clients/KernelClient.hpp>
#include <Quantum/ABI.hpp>

namespace Quantum::Servers::Core {
  class RequestController;

  /**
   * @brief Base class for all servers in QuantumOS.
   *
   * Servers register @ref RequestController instances for specific
   * operation codes via @ref RegisterController. The default
   * @ref ProcessNextMessage implementation reads the operation code
   * from the @ref ABIRequest payload and dispatches to the matching
   * controller.
   *
   * Subclasses may still override @ref ProcessNextMessage for custom
   * dispatch logic. The constructor opens a managed IPC port on the
   * given port ID, and @ref Run provides the standard receive loop.
   *
   * @code
   *   class MyServer : public Server {
   *     public:
   *       MyServer(MyController& c)
   *         : Server(MyPortID)
   *       {
   *         RegisterController(
   *           static_cast<UInt32>(MyOperation::Foo), c
   *         );
   *       }
   *   };
   * @endcode
   */
  class Server {
    private:
      /**
       * @brief Kernel client owned by this server for its own IPC and
       *        memory operations.
       *
       * Declared first so that initializers for later members (e.g.
       * @ref _portHandle) can safely use it.
       */
      Clients::KernelClient _kernel;

    public:
      /**
       * @brief Constructs a server and opens a managed IPC port.
       * @param portID The IPC port ID to listen on.
       */
      Server(IPCPortID portID);

      /**
       * @brief Destroys the server, closing the port handle.
       */
      virtual ~Server();

      /**
       * @brief Enters the server main loop.
       *
       * Blocks on @ref _portHandle, dispatches each received message to
       * @ref ProcessNextMessage, and frees the message afterward. Exits
       * when @ref ProcessNextMessage returns a non-zero termination code.
       *
       * @return The non-zero termination code from
       *         @ref ProcessNextMessage, or 0 if the loop was never
       *         entered.
       */
      Int32 Run();

      /**
       * @brief Registers a @ref RequestController for a specific
       *        operation code.
       * @param operation The operation code (cast from the server's
       *                  operation enum to `UInt32`).
       * @param controller Reference to the controller that handles this
       *                   operation. Must outlive this instance.
       *
       * If the dispatch table is full, the registration is silently
       * ignored. If the same operation code is registered twice, the
       * later registration takes precedence.
       */
      void RegisterController(
        UInt32 operation,
        RequestController& controller
      );

      /**
       * @brief Registers a @ref RequestController for a typed operation
       *        code.
       * @tparam OperationType The operation enum type (must be convertible
       *                       to `UInt32`).
       * @param operation The operation code.
       * @param controller Reference to the controller that handles this
       *                   operation. Must outlive this instance.
       */
      template <typename OperationType>
      void RegisterController(
        OperationType operation,
        RequestController& controller
      ) {
        RegisterController(static_cast<UInt32>(operation), controller);
      }

    protected:
      /**
       * @brief Handles a single incoming IPC message.
       *
       * The default implementation extracts the operation code from the
       * @ref ABIRequest payload and dispatches to the registered
       * @ref RequestController. Subclasses may override for custom
       * dispatch logic.
       *
       * @param message The received IPC message.
       * @return 0 to continue processing, or a non-zero termination
       *         code to exit the server loop.
       */
      virtual Int32 ProcessNextMessage(IPCMessage* message);

      /**
       * @brief The IPC port ID this server is listening on.
       */
      IPCPortID _portID;

      /**
       * @brief The IPC port handle for receiving messages.
       */
      IPCPortResourceID _portHandle;

    private:
      /**
       * @brief Maximum number of operation-to-controller mappings.
       */
      static constexpr Size MaxControllerEntries = 64;

      /**
       * @brief Maps a single operation code to its controller.
       */
      struct ControllerEntry {
        /**
         * @brief The operation code.
         */
        UInt32 Operation;

        /**
         * @brief The controller that handles this operation.
         */
        RequestController* Controller;
      };

      /**
       * @brief Dispatch table mapping operation codes to controllers.
       */
      ControllerEntry _controllerEntries[MaxControllerEntries];

      /**
       * @brief Number of active entries in @ref _controllerEntries.
       */
      Size _controllerEntryCount;
  };
}
