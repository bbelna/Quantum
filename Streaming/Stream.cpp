/**
 * @file Streaming/Stream.cpp
 * @brief Implements @ref @QStrm::Stream.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <OSStreamingTypes.hpp>
#include "Stream.hpp"

namespace Quantum::Streaming {
  Stream Stream::Open(const StreamDescriptor& descriptor) {
    return Open(descriptor.BufferID);
  }

  Stream Stream::Open(SharedBufferID bufferID) {
    Stream stream;

    if (bufferID == 0) return stream;

    UIntPtr address = AttachShared(bufferID);

    if (address == 0) return stream;

    stream._bufferAddress = address;
    stream._bufferID = bufferID;
    stream._header = reinterpret_cast<StreamHeader*>(address);

    return stream;
  }

  Stream::~Stream() {
    if (_header) Close();
  }

  Size Stream::Write(const void* data, Size size) {
    if (!_header || size == 0) return 0;

    const UInt8* source = static_cast<const UInt8*>(data);
    UInt8* ring = _header->Data();
    UInt32 capacity = _header->Capacity;
    Size written = 0;

    while (written < size) {
      // spin-wait if the ring is full; bail out if the other end closed
      while (_header->Free() == 0) {
        if (_header->Closed != 0) return written;

        Thread::Yield();
      }

      UInt32 writeOffset = _header->WriteOffset;
      UInt32 available = _header->Free();
      UInt32 toWrite = static_cast<UInt32>(size - written);

      if (toWrite > available) toWrite = available;

      // write in up to two chunks (wrap around end of ring)
      UInt32 firstChunk = capacity - writeOffset;

      if (firstChunk > toWrite) firstChunk = toWrite;

      for (UInt32 index = 0; index < firstChunk; ++index) {
        ring[writeOffset + index] = source[written + index];
      }

      UInt32 secondChunk = toWrite - firstChunk;

      for (UInt32 index = 0; index < secondChunk; ++index) {
        ring[index] = source[written + firstChunk + index];
      }

      _header->WriteOffset = (writeOffset + toWrite) % capacity;
      written += toWrite;
    }

    return written;
  }

  Size Stream::Write(const char* text) {
    if (!_header || !text) return 0;

    return Write(text, CString::Length(text));
  }

  Size Stream::WriteLine(const char* text) {
    Size written = Write(text);

    written += Write("\n", 1);

    return written;
  }

  Size Stream::Read(void* buffer, Size bufferSize) {
    if (!_header || bufferSize == 0) return 0;

    UInt32 available = _header->Available();

    if (available == 0) return 0;

    UInt8* destination = static_cast<UInt8*>(buffer);
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
        destination[totalRead + index] = ring[
          (readOffset + index) % capacity
        ];
      }

      readOffset = (readOffset + chunkSize) % capacity;
      available -= chunkSize;
      totalRead += chunkSize;
      remaining -= chunkSize;
    }

    _header->ReadOffset = readOffset;

    return totalRead;
  }

  bool Stream::ReadByte(UInt8& outByte) {
    if (!_header) return false;

    for (;;) {
      if (_header->Available() > 0) {
        const UInt8* ring = _header->Data();
        UInt32 readOffset = _header->ReadOffset;

        outByte = ring[readOffset % _header->Capacity];

        _header->ReadOffset = (readOffset + 1) % _header->Capacity;

        return true;
      }

      if (_header->Closed != 0) return false;

      Thread::Yield();
    }
  }

  void Stream::Close() {
    if (_header) {
      _header->Closed = 1;

      DetachShared(_bufferAddress);

      if (_bufferID != 0) {
        StreamCloseWriterRequest request {};

        request.ABIVersion = StreamABIVersion;
        request.Operation = StreamOperation::CloseWriter;
        request.BufferID = _bufferID;

        SendOS(StreamPortID, request);
      }

      _header = nullptr;
      _bufferAddress = 0;
      _bufferID = 0;
    }
  }
}
