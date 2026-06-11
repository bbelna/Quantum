/**
 * @file Streaming/StreamDescriptor.cpp
 * @brief Implements @ref @QStrm::StreamDescriptor.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <StreamingTypes.hpp>

namespace Quantum::Streaming {
  bool StreamDescriptor::Parse(
    const char* argument,
    StreamDescriptor& outDescriptor
  ) {
    // expected format: "stream:N"
    if (!argument) return false;

    if (
      argument[0] != 's' || argument[1] != 't' ||
      argument[2] != 'r' || argument[3] != 'e' ||
      argument[4] != 'a' || argument[5] != 'm' ||
      argument[6] != ':'
    ) {
      return false;
    }

    const char* cursor = argument + 7;

    if (*cursor < '0' || *cursor > '9') return false;

    UInt32 bufferID = 0;

    while (*cursor >= '0' && *cursor <= '9') {
      bufferID = bufferID * 10 + static_cast<UInt32>(*cursor - '0');
      ++cursor;
    }

    outDescriptor.BufferID = bufferID;

    return true;
  }

  void StreamDescriptor::Format(
    const StreamDescriptor& descriptor,
    char* buffer,
    Size bufferSize
  ) {
    CString::Format(
      buffer,
      bufferSize,
      "stream:%u",
      static_cast<UInt32>(descriptor.BufferID)
    );
  }
}
