/**
 * @file Include/Quantum/Servers/Core/RequestController.hpp
 * @brief Declares @ref @QSrvCore::RequestController.
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
  namespace {
    using namespace Quantum::Clients;
  }

  class ServerLog;

  /**
   * @brief Abstract base class for @ref Server request controllers.
   *
   * @ref Server registers one or more @ref RequestController instances, each
   * mapped to a set of operation codes. When an @ref IPCMessage arrives, the
   * @ref Server looks up the operation code from the @ref ABIRequest
   * payload and dispatches to the matching controller's @ref Handle
   * method.
   *
   * Subclasses should override @ref Handle to switch on the operation
   * code and delegate to per-operation handler methods.
   *
   * @code
   *   class VolumeController : public RequestController {
   *     public:
   *       VolumeController(KernelClient& k, ServerLog& l, VolumeTable& v)
   *         : RequestController(k, l), _volumeTable(v) {}
   *
   *       void Handle(UInt32 operation, const IPCMessage* m) override;
   *
   *     private:
   *       VolumeTable& _volumeTable;
   *   };
   * @endcode
   */
  class RequestController {
    public:
      /**
       * @brief Creates a new @ref RequestController.
       * @param kernel
       *   Reference to the @ref KernelClient.
       *   Must outlive this controller.
       * @param log
       *   Reference to the @ref ServerLog for logging.
       *   Must outlive this controller.
       */
      RequestController(
        KernelClient& kernel,
        ServerLog& log
      );

      /**
       * @brief Creates a new @ref RequestController with pre-registered
       *        typed handlers.
       *
       * Each argument after @p kernel and @p log is a @ref Route
       * descriptor binding an operation code to a typed handler.
       *
       * @tparam FirstRoute  First route descriptor type.
       * @tparam MoreRoutes  Additional route descriptor types.
       * @param kernel
       *   Reference to the @ref KernelClient.
       *   Must outlive this controller.
       * @param log
       *   Reference to the @ref ServerLog for logging.
       *   Must outlive this controller.
       * @param first  The first route to register.
       * @param rest   Additional routes to register.
       *
       * @code
       *   MyController(KernelClient& k, ServerLog& l)
       *     : RequestController(
       *         k, l,
       *         Route<&MyController::_handleFoo>(MyOp::Foo),
       *         Route<&MyController::_handleBar>(MyOp::Bar)
       *       )
       *   {}
       * @endcode
       */
      template <typename FirstRoute, typename... MoreRoutes>
      RequestController(
        KernelClient& kernel,
        ServerLog& log,
        FirstRoute first,
        MoreRoutes... rest
      ) :
        _kernel(kernel),
        _log(log)
      {
        _registerRoute(first);
        (_registerRoute(rest), ...);
      }

      /**
       * @brief Destroys the @ref RequestController.
       */
      virtual ~RequestController() = default;

      /**
       * @brief Handles an incoming @ref IPCMessage for a specific @p operation.
       * @param operation
       *   The operation code extracted from the message's @ref ABIRequest
       *   payload (cast to `UInt32`).
       * @param message
       *   The received @ref IPCMessage. Guaranteed non-null with a valid
       *   payload of at least `sizeof(ABIRequest<>)` bytes.
       *
       * The default implementation dispatches to typed handlers registered
       * via @ref On. Subclasses may override for custom dispatch logic;
       * the override can call @c RequestController::Handle to fall through
       * to the typed handler table.
       */
      virtual void Handle(UInt32 operation, const IPCMessage* message);

    protected:
      /**
       * @brief Reference to the @ref KernelClient.
       */
      KernelClient& _kernel;

      /**
       * @brief Reference to the @ref ServerLog for logging.
       */
      ServerLog& _log;

      /**
       * @brief Sends a reply @p payload to a client's reply port.
       * @param replyPortID The client's reply @ref IPCPortID.
       * @param payload Opaque `void` pointer to the payload to send.
       * @param payloadSizeInBytes @ref Size of the payload in bytes.
       */
      void SendReply(
        IPCPortID replyPortID,
        const void* payload,
        Size payloadSizeInBytes
      );

      /**
       * @brief Registers a typed handler for a specific operation.
       *
       * When the default @ref Handle implementation receives a message
       * with the matching operation code, it validates that the payload
       * is at least @c sizeof(RequestType) bytes, casts it, and invokes
       * the handler.
       *
       * @tparam Handler  Pointer-to-member-function on the derived
       *                  controller. Accepted signatures:
       *                  - @c void(const RequestType&)
       *                  - @c void(const RequestType&, const IPCMessage*)
       * @tparam OperationType The operation enum type.
       * @param operation The operation code to handle.
       *
       * @code
       *   // In the derived controller's constructor:
       *   On<&LoadController::_handleLoadELF>(RunServerOperation::LoadELF);
       * @endcode
       */
      template <auto Handler, typename OperationType>
      void On(OperationType operation) {
        using Traits = _HandlerTraits<decltype(Handler)>;
        using Controller = typename Traits::ClassType;
        using RequestType = typename Traits::RequestType;

        _addHandler(
          static_cast<UInt32>(operation),
          sizeof(RequestType),
          [](RequestController* self, const IPCMessage* message) {
            const RequestType& request
              = *reinterpret_cast<const RequestType*>(message->Payload);

            if constexpr (Traits::PassesMessage) {
              (static_cast<Controller*>(self)->*Handler)(request, message);
            } else {
              (static_cast<Controller*>(self)->*Handler)(request);
            }
          }
        );
      }

      /**
       * @brief Pairs a handler pointer with an operation code for
       *        constructor-based route registration.
       * @tparam Handler  Pointer-to-member-function on the derived
       *                  controller.
       */
      template <auto Handler>
      struct _RouteDescriptor {
        UInt32 Operation;
      };

      /**
       * @brief Creates a @ref _RouteDescriptor binding a handler to an
       *        operation code.
       *
       * @tparam Handler  Pointer-to-member-function on the derived
       *                  controller. Same signatures accepted by @ref On.
       * @tparam OperationType  The operation enum type.
       * @param operation  The operation code to handle.
       *
       * @code
       *   Route<&MyController::_handleFoo>(MyOp::Foo)
       * @endcode
       */
      template <auto Handler, typename OperationType>
      static constexpr _RouteDescriptor<Handler> Route(
        OperationType operation
      ) {
        return { static_cast<UInt32>(operation) };
      }

    private:
      /**
       * @brief Type-erased handler function pointer.
       */
      using _HandlerInvokeFn
        = void (*)(RequestController*, const IPCMessage*);

      /**
       * @brief A single typed handler entry in the dispatch table.
       */
      struct _TypedHandler {
        UInt32 Operation;
        Size MinPayloadSize;
        _HandlerInvokeFn Invoke;
      };

      /**
       * @brief Extracts the class and request types from a member
       *        function pointer.
       */
      template <typename T> struct _HandlerTraits;

      template <typename C, typename R>
      struct _HandlerTraits<void (C::*)(const R&)> {
        using ClassType = C;
        using RequestType = R;
        static constexpr bool PassesMessage = false;
      };

      template <typename C, typename R>
      struct _HandlerTraits<void (C::*)(const R&, const IPCMessage*)> {
        using ClassType = C;
        using RequestType = R;
        static constexpr bool PassesMessage = true;
      };

      /**
       * @brief Registers a single route from a @ref _RouteDescriptor.
       */
      template <auto Handler>
      void _registerRoute(_RouteDescriptor<Handler> route) {
        On<Handler>(route.Operation);
      }

      static constexpr Size MaxTypedHandlers = 16;

      _TypedHandler _typedHandlers[MaxTypedHandlers] = {};
      Size _typedHandlerCount = 0;

      void _addHandler(
        UInt32 operation,
        Size minPayloadSize,
        _HandlerInvokeFn invoke
      );
  };
}
