/**
 * @file Include/Quantum/Streaming/StreamDescriptor.hpp
 * @brief Declares @ref Quantum::Streaming::StreamDescriptor.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>

namespace Quantum::Streaming {
  /**
   * @brief Maximum length of a serialized stream descriptor string,
   *        including the null terminator.
   */
  static constexpr Size MaxDescriptorLength = 32;

  /**
   * @brief Describes a shared-memory stream channel between a host
   *        (e.g. the shell) and a client (e.g. a terminal utility).
   *
   * The descriptor contains the ID of a shared buffer that holds a
   * @ref StreamHeader followed by a circular byte buffer. It is
   * serialized to a compact string for passing via argv.
   */
  struct StreamDescriptor {
    /**
     * @brief The shared buffer ID containing the stream ring buffer.
     */
    UInt32 BufferID = 0;

    /**
     * @brief Parses a stream descriptor from an argv string.
     * @param argument The string to parse (format: `"stream:N"`).
     * @param outDescriptor Receives the parsed descriptor on success.
     * @return `true` if parsing succeeded; `false` otherwise.
     */
    static bool Parse(
      const char* argument,
      StreamDescriptor& outDescriptor
    );

    /**
     * @brief Formats a stream descriptor into an argv string.
     * @param descriptor The descriptor to serialize.
     * @param buffer Output buffer for the formatted string.
     * @param bufferSize Size of the output buffer in bytes.
     */
    static void Format(
      const StreamDescriptor& descriptor,
      char* buffer,
      Size bufferSize
    );
  };
}
