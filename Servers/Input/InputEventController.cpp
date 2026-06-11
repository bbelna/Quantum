/**
 * @file Servers/Input/InputEventController.cpp
 * @brief Implements @ref @QInSrv::InputEventController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "InputEventController.hpp"

namespace Quantum::Servers::Input {
  InputEventController::InputEventController(
    KernelClient& kernel,
    ServerLog& log
  ) :
    RequestController(kernel, log)
  {
  }

  void InputEventController::Handle(
    UInt32 operation,
    const IPCMessage* message
  ) {
    switch (Enum::To<InputOperation>(operation)) {
      case InputOperation::ReportEvent: {
        _handleReportEvent(message);

        break;
      }

      case InputOperation::GetNextEvent: {
        _handleGetNextEvent(message);

        break;
      }

      case InputOperation::TryGetNextEvent: {
        _handleTryGetNextEvent(message);

        break;
      }

      default: break;
    }
  }

  void InputEventController::_handleReportEvent(const IPCMessage* message) {
    if (message->PayloadSizeInBytes < sizeof(InputReportEventRequest)) {
      _log.Warning("Input: ReportEvent dropped: undersized message");

      return;
    }

    auto* request = reinterpret_cast<const InputReportEventRequest*>(
      message->Payload
    );

    InputStream* stream = _findOrCreateStream(request->Event.SourceDeviceID);

    if (!stream) {
      _log.Warning(
        "No available stream slot for device %u",
        request->Event.SourceDeviceID
      );

      return;
    }

    stream->Push(request->Event);

    if (_pendingReader.Active) {
      InputEvent event;

      if (stream->Pop(&event)) {
        _deliverToPendingReader(event);
      }
    }
  }

  void InputEventController::_handleGetNextEvent(const IPCMessage* message) {
    if (message->PayloadSizeInBytes < sizeof(InputGetNextEventRequest)) {
      _log.Warning("Input: GetNextEvent dropped: undersized message");

      return;
    }

    auto* request = reinterpret_cast<const InputGetNextEventRequest*>(
      message->Payload
    );

    for (Size index = 0; index < MaxInputStreams; index++) {
      if (_streams[index].HasEvents()) {
        InputEvent event;

        if (_streams[index].Pop(&event)) {
          InputGetNextEventResult result { true, event };

          SendReply(request->ReplyPortID, &result, sizeof(result));

          return;
        }
      }
    }

    _pendingReader.ReplyPortID = request->ReplyPortID;
    _pendingReader.Active = true;
  }

  void InputEventController::_handleTryGetNextEvent(
    const IPCMessage* message
  ) {
    if (message->PayloadSizeInBytes < sizeof(InputGetNextEventRequest)) {
      _log.Warning("Input: TryGetNextEvent dropped: undersized message");

      return;
    }

    auto* request = reinterpret_cast<const InputGetNextEventRequest*>(
      message->Payload
    );

    for (Size index = 0; index < MaxInputStreams; index++) {
      if (_streams[index].HasEvents()) {
        InputEvent event;

        if (_streams[index].Pop(&event)) {
          InputGetNextEventResult result { true, event };

          SendReply(request->ReplyPortID, &result, sizeof(result));

          return;
        }
      }
    }

    InputGetNextEventResult result { false, {} };

    SendReply(request->ReplyPortID, &result, sizeof(result));
  }

  bool InputEventController::_deliverToPendingReader(const InputEvent& event) {
    if (!_pendingReader.Active) return false;

    InputGetNextEventResult result { true, event };

    SendReply(_pendingReader.ReplyPortID, &result, sizeof(result));

    _pendingReader.Active = false;

    return true;
  }

  InputStream* InputEventController::_findOrCreateStream(DeviceID deviceID) {
    for (Size index = 0; index < MaxInputStreams; index++) {
      if (_streams[index].SourceDeviceID == deviceID) {
        return &_streams[index];
      }
    }

    for (Size index = 0; index < MaxInputStreams; index++) {
      if (_streams[index].SourceDeviceID == 0) {
        _streams[index].SourceDeviceID = deviceID;

        _log.Trace(
          "Created input stream for device %u (slot %u)",
          deviceID,
          index
        );

        return &_streams[index];
      }
    }

    return nullptr;
  }
}
