/**
 * @file Include/Quantum/Streaming/StreamHeader.hpp
 * @brief Declares @ref Quantum::Streaming::StreamHeader.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Streaming {
  /**
   * @brief Default size of the shared stream buffer (header + ring data).
   */
  static constexpr Size DefaultStreamBufferSize = 4096;

  /**
   * @brief Header at the start of a shared-memory stream buffer. Followed
   *        immediately by @ref Capacity bytes of circular ring data.
   *
   * The child (writer) advances @ref WriteOffset; the host (reader)
   * advances @ref ReadOffset. Both wrap modulo @ref Capacity.
   */
  struct StreamHeader {
    /**
     * @brief Byte offset into the ring where the next write occurs.
     *        Advanced by the writer (child process).
     */
    volatile UInt32 WriteOffset;

    /**
     * @brief Byte offset into the ring where the next read occurs.
     *        Advanced by the reader (host process).
     */
    volatile UInt32 ReadOffset;

    /**
     * @brief Set to 1 by the writer when it has finished writing and
     *        closed the stream. The host drains remaining data and stops.
     */
    volatile UInt32 Closed;

    /**
     * @brief Size of the ring data area in bytes (immediately after this
     *        header).
     */
    UInt32 Capacity;

    /**
     * @brief Returns a pointer to the start of the ring data.
     */
    UInt8* Data() {
      return reinterpret_cast<UInt8*>(this) + sizeof(StreamHeader);
    }

    /**
     * @brief Returns a const pointer to the start of the ring data.
     */
    const UInt8* Data() const {
      return reinterpret_cast<const UInt8*>(this) + sizeof(StreamHeader);
    }

    /**
     * @brief Returns the number of bytes available to read.
     */
    UInt32 Available() const {
      return (WriteOffset - ReadOffset) % Capacity;
    }

    /**
     * @brief Returns the number of bytes of free space for writing.
     *        One byte is reserved to distinguish full from empty.
     */
    UInt32 Free() const {
      return Capacity - 1 - Available();
    }
  };
}
