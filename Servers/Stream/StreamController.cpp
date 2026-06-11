/**
 * @file Servers/Stream/StreamController.cpp
 * @brief Implements @ref @QStrmSrv::StreamController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "StreamController.hpp"

namespace Quantum::Servers::Stream {
  StreamController::StreamController(
    KernelClient& kernel,
    ServerLog& log
  ) : RequestController(kernel, log) {}

  void StreamController::Handle(
    UInt32 operation,
    const IPCMessage* message
  ) {
    switch (Enum::To<StreamOperation>(operation)) {
      case StreamOperation::CreateStream: {
        _handleCreateStream(message);

        break;
      }

      case StreamOperation::CloseWriter: {
        _handleCloseWriter(message);

        break;
      }

      case StreamOperation::CloseReader: {
        _handleCloseReader(message);

        break;
      }

      default: break;
    }
  }

  void StreamController::_handleCreateStream(const IPCMessage* message) {
    if (message->PayloadSizeInBytes < sizeof(StreamCreateRequest)) {
      _log.Warning("CreateStream dropped: undersized message");

      return;
    }

    const StreamCreateRequest* request = Cast::As<StreamCreateRequest>(
      message->Payload
    );

    ManagedStream* slot = _findFreeSlot();

    if (!slot) {
      _log.Warning("CreateStream failed: no free slots");

      StreamCreateResult result {
        false,
        0
      };

      SendReply(
        request->ReplyPortID,
        &result,
        sizeof(result)
      );

      return;
    }

    Size bufferSize = request->BufferSize > 0
      ? static_cast<Size>(request->BufferSize)
      : DefaultStreamBufferSize;

    SharedBufferID bufferID = _kernel.CreateSharedBuffer(bufferSize);

    if (bufferID == 0) {
      _log.Warning("CreateStream failed: could not create shared buffer");

      StreamCreateResult result {
        false,
        0
      };

      SendReply(
        request->ReplyPortID,
        &result,
        sizeof(result)
      );

      return;
    }

    UIntPtr bufferAddress = _kernel.AttachSharedBuffer(bufferID);

    if (bufferAddress == 0) {
      _log.Warning("CreateStream failed: could not attach shared buffer");

      StreamCreateResult result {
        false,
        0
      };

      SendReply(
        request->ReplyPortID,
        &result,
        sizeof(result)
      );

      return;
    }

    StreamHeader* header = reinterpret_cast<StreamHeader*>(bufferAddress);

    header->WriteOffset = 0;
    header->ReadOffset = 0;
    header->Closed = 0;
    header->Capacity = static_cast<UInt32>(bufferSize - sizeof(StreamHeader));

    UInt8* ringData = header->Data();

    for (UInt32 index = 0; index < header->Capacity; ++index) {
      ringData[index] = 0;
    }

    slot->BufferID = bufferID;
    slot->BufferAddress = bufferAddress;
    slot->WriterClosed = false;
    slot->ReaderClosed = false;

    _log.Trace(
      "Created stream (buffer %u, %u bytes)",
      bufferID,
      static_cast<UInt32>(bufferSize)
    );

    StreamCreateResult result {
      true,
      bufferID
    };

    SendReply(
      request->ReplyPortID,
      &result,
      sizeof(result)
    );
  }

  void StreamController::_handleCloseWriter(const IPCMessage* message) {
    if (message->PayloadSizeInBytes < sizeof(StreamCloseWriterRequest)) {
      return;
    }

    const StreamCloseWriterRequest* request = Cast::As<StreamCloseWriterRequest>(
      message->Payload
    );
    ManagedStream* stream = _findStream(request->BufferID);

    if (!stream) {
      return;
    } else {
      stream->WriterClosed = true;

      _tryCleanup(stream);
    }
  }

  void StreamController::_handleCloseReader(const IPCMessage* message) {
    if (
      message->PayloadSizeInBytes < sizeof(StreamCloseReaderRequest)
    ) {
      return;
    }

    const StreamCloseReaderRequest* request = Cast::As<StreamCloseReaderRequest>(
      message->Payload
    );
    ManagedStream* stream = _findStream(request->BufferID);

    if (!stream) {
      return;
    } else {
      stream->ReaderClosed = true;

      _tryCleanup(stream);
    }
  }

  ManagedStream* StreamController::_findStream(SharedBufferID bufferID) {
    for (Size index = 0; index < MaxStreams; ++index) {
      if (_streams[index].BufferID == bufferID) {
        return &_streams[index];
      }
    }

    return nullptr;
  }

  ManagedStream* StreamController::_findFreeSlot() {
    for (Size index = 0; index < MaxStreams; ++index) {
      if (_streams[index].BufferID == 0) {
        return &_streams[index];
      }
    }

    return nullptr;
  }

  void StreamController::_tryCleanup(ManagedStream* stream) {
    if (!stream->WriterClosed || !stream->ReaderClosed) {
      return;
    } else {
      _log.Trace(
        "Cleaned up stream (buffer %u)",
        stream->BufferID
      );

      if (stream->BufferAddress != 0) {
        _kernel.DetachSharedBuffer(stream->BufferAddress);
      }

      stream->BufferID = 0;
      stream->BufferAddress = 0;
      stream->WriterClosed = false;
      stream->ReaderClosed = false;
    }
  }
}
