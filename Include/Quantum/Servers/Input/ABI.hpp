/**
 * @file Include/Quantum/Servers/Input/ABI.hpp
 * @brief Declares the input server's ABI types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>
#include <Quantum/Input.hpp>
#include <Quantum/ABI.hpp>
#include <Quantum/Types.hpp>

namespace Quantum::Servers::Input::ABI {
  /**
   * @brief ABI version for the input server protocol.
   */
  constexpr UInt32 InputABIVersion = 2;

  /**
   * @brief IPC port ID for the input server.
   */
  constexpr IPCPortID InputPortID = 5;

  /**
   * @brief Operations supported by the input server.
   */
  enum class InputOperation : UInt32 {
    /**
     * @brief Reports a single input event from a device driver.
     */
    ReportEvent = 1,

    /**
     * @brief Retrieves the next input event. Blocks until an event is
     *        available if no events are currently buffered.
     */
    GetNextEvent = 2,

    /**
     * @brief Non-blocking variant of GetNextEvent. Returns immediately
     *        with HasEvent=false if no events are currently buffered.
     */
    TryGetNextEvent = 3
  };

  /**
   * @brief Base request type for fire-and-forget input operations.
   */
  using InputRequest = ABIRequest<InputOperation>;

  /**
   * @brief Base request type for input operations that expect a reply.
   */
  using InputRequestWithReply = ABIRequestWithReplyPort<InputOperation>;

  /**
   * @brief Request to report a single input event.
   */
  struct InputReportEventRequest : public InputRequest {
    /**
     * @brief The input event to report.
     */
    Quantum::Input::InputEvent Event;
  };

  /**
   * @brief Request to get the next input event. Blocks until available.
   */
  struct InputGetNextEventRequest : public InputRequestWithReply {};

  /**
   * @brief Request to try to get the next input event. Non-blocking.
   */
  using InputTryGetNextEventRequest = InputGetNextEventRequest;

  /**
   * @brief Result of a @ref InputOperation::GetNextEvent or
   *        @ref InputOperation::TryGetNextEvent request.
   */
  struct InputGetNextEventResult {
    /**
     * @brief `true` if an event is present; `false` otherwise.
     */
    bool HasEvent;

    /**
     * @brief The input event. Valid only when `HasEvent` is `true`.
     */
    Quantum::Input::InputEvent Event;
  };
}
