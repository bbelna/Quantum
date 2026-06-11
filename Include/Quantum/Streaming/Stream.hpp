/**
 * @file Include/Quantum/Streaming/Stream.hpp
 * @brief Declares @ref Quantum::Streaming::Stream.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>
#include <Quantum/Kernel/Memory/MemoryTypes.hpp>
#include <Quantum/Streaming/StreamDescriptor.hpp>
#include <Quantum/Streaming/StreamHeader.hpp>

namespace Quantum::Streaming {
  /**
   * @brief Client-side stream endpoint. Opened by a child process (e.g. a
   *        terminal utility) from a @ref Quantum::Streaming::StreamDescriptor
   *        received via argv.
   *
   * The stream is backed by a shared-memory ring buffer. Writes copy data
   * into the ring; the host polls and reads from the other side.
   */
  class Stream {
    public:
      /**
       * @brief Opens a stream by attaching to the shared buffer described
       *        by the given descriptor.
       * @param descriptor The stream descriptor containing the buffer ID.
       * @return A stream instance. Check @ref IsValid before use.
       */
      static Stream Open(
        const Quantum::Streaming::StreamDescriptor& descriptor
      );

      /**
       * @brief Opens a stream by attaching to the shared buffer with the
       *        given ID.
       * @param bufferID The shared buffer ID to attach to.
       * @return A stream instance. Check @ref IsValid before use.
       */
      static Stream Open(Quantum::Kernel::Memory::SharedBufferID bufferID);

      /**
       * @brief Constructs an invalid (closed) stream.
       */
      Stream() = default;

      /**
       * @brief Destroys the stream, marking it as closed.
       */
      ~Stream();

      /**
       * @brief Move-constructs a stream, transferring ownership from
       *        @p other. The source stream becomes invalid after the move.
       * @param other The stream to move from.
       */
      Stream(Stream&& other)
        : _header(other._header)
        , _bufferAddress(other._bufferAddress)
        , _bufferID(other._bufferID)
      {
        other._header = nullptr;
        other._bufferAddress = 0;
        other._bufferID = 0;
      }

      /**
       * @brief Move-assigns a stream, transferring ownership from
       *        @p other. Any previously held stream is closed first.
       *        The source stream becomes invalid after the move.
       * @param other The stream to move from.
       * @return Reference to this stream.
       */
      Stream& operator=(Stream&& other) {
        if (this != &other) {
          if (_header) Close();

          _header = other._header;
          _bufferAddress = other._bufferAddress;
          _bufferID = other._bufferID;

          other._header = nullptr;
          other._bufferAddress = 0;
          other._bufferID = 0;
        }

        return *this;
      }

      /**
       * @brief Copying is not allowed - streams have unique ownership.
       */
      Stream(const Stream&) = delete;

      /**
       * @brief Copy-assignment is not allowed - streams have unique
       *        ownership.
       */
      Stream& operator=(const Stream&) = delete;

      /**
       * @brief Returns whether this stream was opened successfully.
       */
      bool IsValid() const { return _header != nullptr; }

      /**
       * @brief Writes raw bytes to the stream.
       * @param data Pointer to the data to write.
       * @param size Number of bytes to write.
       * @return Number of bytes written.
       */
      Size Write(const void* data, Size size);

      /**
       * @brief Writes a null-terminated string to the stream.
       * @param text The string to write.
       * @return Number of bytes written.
       */
      Size Write(const char* text);

      /**
       * @brief Writes a null-terminated string followed by a newline.
       * @param text The string to write.
       * @return Number of bytes written (including the newline).
       */
      Size WriteLine(const char* text);

      /**
       * @brief Reads raw bytes from the stream (non-blocking).
       *
       * Copies up to @p bufferSize bytes that are currently available in
       * the ring buffer into @p buffer. Returns immediately with 0 if no
       * data is available.
       *
       * @param buffer Destination buffer to receive the data.
       * @param bufferSize Maximum number of bytes to read.
       * @return Number of bytes actually read.
       */
      Size Read(void* buffer, Size bufferSize);

      /**
       * @brief Reads a single byte from the stream (blocking).
       *
       * Spins with @ref Quantum::Threading::Thread::Yield until at least
       * one byte is available or the stream is closed.
       *
       * @param outByte Receives the byte that was read.
       * @return `true` if a byte was read, `false` if the stream was
       *         closed with no remaining data.
       */
      bool ReadByte(UInt8& outByte);

      /**
       * @brief Marks the stream as closed and detaches the shared buffer.
       */
      void Close();

    private:
      /**
       * @brief Pointer to the stream header in shared memory.
       */
      Quantum::Streaming::StreamHeader* _header = nullptr;

      /**
       * @brief The mapped address of the shared buffer (for detaching).
       */
      UIntPtr _bufferAddress = 0;

      /**
       * @brief The shared buffer ID (for sending CloseWriter on close).
       */
      UInt32 _bufferID = 0;
  };
}
