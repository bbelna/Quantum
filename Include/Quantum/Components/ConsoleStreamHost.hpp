/**
 * @file Include/Quantum/Components/ConsoleStreamHost.hpp
 * @brief Declares @ref Quantum::UI::StreamHost.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>
#include <Quantum/Memory.hpp>
#include <Quantum/Kernel.hpp>
#include <Quantum/Streaming/StreamDescriptor.hpp>
#include <Quantum/Streaming/StreamHeader.hpp>

namespace Quantum::Components {
  class Console;

  /**
   * @brief Host-side stream endpoint for reading a child process's output
   *        from a shared-memory ring buffer.
   *
   * Two usage modes are supported:
   *
   *   1. **Console-bridged** (legacy): construct with a @ref Console
   *      reference, call @ref Create to allocate the buffer, and @ref Poll
   *      to read data directly into the console.
   *
   *   2. **Standalone**: default-construct, call @ref AttachToBuffer with a
   *      pre-existing shared buffer ID (e.g. one created by the stream
   *      server), and use @ref Read to pull data into a caller-owned buffer.
   */
  class ConsoleStreamHost {
    public:
      /**
       * @brief Constructs a standalone stream host with no console.
       */
      ConsoleStreamHost();

      /**
       * @brief Constructs a stream host bridged to the given console.
       * @param console The console element that receives child output.
       */
      explicit ConsoleStreamHost(Console& console);

      /**
       * @brief Destroys the stream host, releasing the shared buffer.
       */
      ~ConsoleStreamHost();

      /**
       * @brief Creates the shared buffer for the stream channel.
       * @param outDescriptor Receives the descriptor to pass to the child.
       * @return `true` if the buffer was created successfully.
       */
      bool Create(Quantum::Streaming::StreamDescriptor& outDescriptor);

      /**
       * @brief Attaches to a pre-existing shared buffer (e.g. one created
       *        by the stream server).
       * @param bufferID The shared buffer ID to attach.
       * @return `true` if the buffer was attached successfully.
       */
      bool AttachToBuffer(SharedBufferID bufferID);

      /**
       * @brief Reads available data from the ring buffer into a
       *        caller-owned buffer. Non-blocking.
       * @param buffer Destination buffer.
       * @param bufferSize Maximum number of bytes to read.
       * @return Number of bytes actually read.
       */
      Size Read(char* buffer, Size bufferSize);

      /**
       * @brief Reads any new data from the ring buffer and writes it to
       *        the console. Non-blocking. Requires a console-bridged
       *        host.
       * @return `true` if data was read; `false` if no data was available.
       */
      bool Poll();

      /**
       * @brief Returns whether the child has closed its end of the stream.
       */
      bool IsClosed() const;

      /**
       * @brief Releases the shared buffer.
       */
      void Close();

      /**
       * @brief Returns whether this host was created successfully.
       */
      bool IsValid() const { return _header != nullptr; }

    private:
      /**
       * @brief The @ref Console that receives child output, or `nullptr`
       *        for standalone mode.
       */
      Console* _console = nullptr;

      /**
       * @brief Pointer to the @ref Quantum::Streaming::StreamHeader in
       *        shared memory.
       */
      Quantum::Streaming::StreamHeader* _header = nullptr;

      /**
       * @brief The mapped address of the shared buffer (for detaching).
       */
      UIntPtr _bufferAddress = 0;

      /**
       * @brief The @ref SharedBufferID (for the descriptor).
       */
      SharedBufferID _bufferID = 0;
  };
}
