/**
 * @file Core/Byte.cpp
 * @brief Implements @ref @QCore::Byte.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "Byte.hpp"

namespace Quantum::Core::Byte {
  void Copy(
    void* destination,
    const void* source,
    Size length
  ) {
    if (
      destination &&
      source &&
      length > 0
    ) {
      UInt8* destinationAsBytes = reinterpret_cast<UInt8*>(destination);
      const UInt8* sourceAsBytes = reinterpret_cast<const UInt8*>(source);

      for (
        Size index = 0;
        index < length;
        ++index
      ) {
        destinationAsBytes[index] = sourceAsBytes[index];
      }
    }
  }

  void Copy(
    UInt8* destination,
    UInt8* source,
    Size length
  ) {
    if (
      destination &&
      source &&
      length > 0
    ) {
      for (
        Size index = 0;
        index < length;
        ++index
      ) {
        destination[index] = source[index];
      }
    }
  }

  void Fill(
    void* destination,
    UInt8 value,
    Size length
  ) {
    if (
      destination &&
      length > 0
    ) {
      volatile UInt8* bytes = reinterpret_cast<volatile UInt8*>(destination);

      for (
        Size index = 0;
        index < length;
        ++index
      ) {
        bytes[index] = value;
      }
    }
  }

  void Zero(
    void* destination,
    Size length
  ) {
    Fill(
      destination,
      0,
      length
    );
  }
}
