/**
 * @file Servers/Input/InputEventController.hpp
 * @brief Declares @ref @QInSrv::InputEventController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <InputServerTypes.hpp>

namespace Quantum::Servers::Input {
  using Quantum::HAL::DeviceID;

  /**
   * @brief Maximum number of concurrent input device streams.
   */
  inline constexpr Size MaxInputStreams = 8;

  /**
   * @brief A ring buffer of input events from a single device.
   *
   * Each active input device is assigned one @ref InputStream. Events
   * are stored in a fixed-capacity circular buffer; when the buffer is
   * full, the oldest event is silently overwritten.
   */
  struct InputStream {
    /**
     * @brief The device ID that this stream is associated with.
     *        A value of `0` indicates an unused stream slot.
     */
    DeviceID SourceDeviceID = 0;

    /**
     * @brief Maximum number of events stored in this stream.
     */
    static constexpr Size Capacity = 64;

    /**
     * @brief Circular buffer of input events.
     */
    InputEvent Events[Capacity] = {};

    /**
     * @brief Next read position in the circular buffer.
     */
    Size Head = 0;

    /**
     * @brief Next write position in the circular buffer.
     */
    Size Tail = 0;

    /**
     * @brief Number of events currently stored (capped at @ref Capacity).
     */
    Size Count = 0;

    /**
     * @brief Pushes an event into the ring buffer. If the buffer is full,
     *        the oldest event is overwritten.
     * @param event The input event to push.
     */
    void Push(const InputEvent& event) {
      Events[Tail] = event;
      Tail = (Tail + 1) % Capacity;

      if (Count < Capacity) {
        Count++;
      } else {
        Head = (Head + 1) % Capacity;
      }
    }

    /**
     * @brief Pops the oldest event from the ring buffer.
     * @param out Receives the popped event.
     * @return `true` if an event was available; `false` if the buffer is
     *         empty.
     */
    bool Pop(InputEvent* out) {
      if (Count == 0) return false;

      *out = Events[Head];
      Head = (Head + 1) % Capacity;
      Count--;

      return true;
    }

    /**
     * @brief Returns whether the buffer has any events.
     */
    bool HasEvents() const { return Count > 0; }
  };

  /**
   * @brief Tracks a process waiting for the next input event.
   */
  struct PendingReader {
    /**
     * @brief IPC port to reply to when an event arrives.
     */
    IPCPortID ReplyPortID = 0;

    /**
     * @brief Whether a reader is currently waiting.
     */
    bool Active = false;
  };

  /**
   * @brief Handles input event operations: ReportEvent, GetNextEvent,
   *        and TryGetNextEvent.
   *
   * Maintains per-device @ref InputStream ring buffers and a single
   * @ref PendingReader slot. When a blocking @ref InputOperation::GetNextEvent
   * request arrives with no buffered events, the caller is parked until the
   * next @ref InputOperation::ReportEvent delivers an event.
   */
  class InputEventController : public RequestController {
    public:
      /**
       * @brief Creates a new @ref InputEventController.
       * @param kernel Reference to the @ref KernelClient for IPC
       *                     operations. Must outlive this controller.
       * @param log Reference to the @ref ServerLog for logging. Must
       *            outlive this controller.
       */
      InputEventController(
        KernelClient& kernel,
        ServerLog& log
      );

      /**
       * @brief Dispatches an input operation to the appropriate handler.
       * @param operation The operation code.
       * @param message The received @ref IPCMessage.
       */
      void Handle(UInt32 operation, const IPCMessage* message) override;

    private:
      /**
       * @brief Per-device input event streams.
       */
      InputStream _streams[MaxInputStreams] = {};

      /**
       * @brief A single pending reader waiting for the next event.
       */
      PendingReader _pendingReader;

      /**
       * @brief Handles a @ref InputOperation::ReportEvent request.
       * @param message The @ref IPCMessage containing the request payload.
       */
      void _handleReportEvent(const IPCMessage* message);

      /**
       * @brief Handles a @ref InputOperation::GetNextEvent request.
       * @param message The @ref IPCMessage containing the request payload.
       */
      void _handleGetNextEvent(const IPCMessage* message);

      /**
       * @brief Handles a @ref InputOperation::TryGetNextEvent request.
       * @param message The @ref IPCMessage containing the request payload.
       */
      void _handleTryGetNextEvent(const IPCMessage* message);

      /**
       * @brief Finds or creates an input stream for the given device ID.
       * @param deviceID The device ID to find or create a stream for.
       * @return Pointer to the stream, or `nullptr` if all slots are full.
       */
      InputStream* _findOrCreateStream(DeviceID deviceID);

      /**
       * @brief Sends an event to the pending reader if one exists.
       * @param event The event to deliver.
       * @return `true` if a pending reader was waiting and received the
       *         event; `false` otherwise.
       */
      bool _deliverToPendingReader(const InputEvent& event);
  };
}
