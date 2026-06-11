/**
 * @file UI/StreamHost.cpp
 * @brief Implements @ref Quantum::UI::StreamHost.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "ComponentTypes.hpp"

namespace Quantum::Components {
  ConsoleStreamHost::ConsoleStreamHost() {}

  ConsoleStreamHost::ConsoleStreamHost(Console& console) : _console(&console) {}

  ConsoleStreamHost::~ConsoleStreamHost() {
    if (_header) Close();
  }

  bool ConsoleStreamHost::Create(
    Quantum::Streaming::StreamDescriptor& outDescriptor
  ) {
    _bufferID = CreateShared(DefaultStreamBufferSize);

    if (_bufferID == 0) return false;

    _bufferAddress = AttachShared(_bufferID);

    if (_bufferAddress == 0) return false;

    _header = reinterpret_cast<StreamHeader*>(_bufferAddress);

    // initialize the ring buffer header
    _header->WriteOffset = 0;
    _header->ReadOffset = 0;
    _header->Closed = 0;
    _header->Capacity = static_cast<UInt32>(
      DefaultStreamBufferSize - sizeof(StreamHeader)
    );

    // zero the ring data
    UInt8* ringData = _header->Data();

    for (UInt32 index = 0; index < _header->Capacity; ++index) {
      ringData[index] = 0;
    }

    outDescriptor.BufferID = _bufferID;

    return true;
  }

  bool ConsoleStreamHost::AttachToBuffer(SharedBufferID bufferID) {
    _bufferID = bufferID;
    _bufferAddress = AttachShared(bufferID);

    if (_bufferAddress == 0) return false;

    _header = reinterpret_cast<StreamHeader*>(_bufferAddress);

    return true;
  }

  Size ConsoleStreamHost::Read(char* buffer, Size bufferSize) {
    if (!_header || bufferSize == 0) return 0;

    UInt32 available = _header->Available();

    if (available == 0) return 0;

    const UInt8* ring = _header->Data();
    UInt32 capacity = _header->Capacity;
    UInt32 readOffset = _header->ReadOffset;
    Size totalRead = 0;
    Size remaining = bufferSize;

    while (available > 0 && remaining > 0) {
      UInt32 chunkSize = available;

      if (chunkSize > remaining) {
        chunkSize = static_cast<UInt32>(remaining);
      }

      for (UInt32 index = 0; index < chunkSize; ++index) {
        buffer[totalRead + index] = static_cast<char>(
          ring[(readOffset + index) % capacity]
        );
      }

      readOffset = (readOffset + chunkSize) % capacity;
      available -= chunkSize;
      totalRead += chunkSize;
      remaining -= chunkSize;
    }

    _header->ReadOffset = readOffset;

    return totalRead;
  }

  bool ConsoleStreamHost::Poll() {
    if (!_header || !_console) return false;

    UInt32 available = _header->Available();

    if (available == 0) return false;

    const UInt8* ring = _header->Data();
    UInt32 capacity = _header->Capacity;
    UInt32 readOffset = _header->ReadOffset;

    // read in chunks, null-terminate for terminal output
    char buffer[256];

    while (available > 0) {
      UInt32 chunkSize = available > 255 ? 255 : available;

      for (UInt32 index = 0; index < chunkSize; ++index) {
        buffer[index] = static_cast<char>(
          ring[(readOffset + index) % capacity]
        );
      }

      buffer[chunkSize] = '\0';

      _console->Write(buffer);

      readOffset = (readOffset + chunkSize) % capacity;
      available -= chunkSize;
    }

    _header->ReadOffset = readOffset;

    return true;
  }

  bool ConsoleStreamHost::IsClosed() const {
    return !_header || _header->Closed != 0;
  }

  void ConsoleStreamHost::Close() {
    if (_bufferAddress != 0) {
      if (_header) _header->Closed = 1;

      DetachShared(_bufferAddress);

      if (_bufferID != 0) {
        using namespace Quantum::Servers::Stream;

        StreamCloseReaderRequest request {};

        request.ABIVersion = StreamABIVersion;
        request.Operation = StreamOperation::CloseReader;
        request.BufferID = _bufferID;

        SendOS(StreamPortID, request);
      }

      _header = nullptr;
      _bufferAddress = 0;
      _bufferID = 0;
    }
  }
}
