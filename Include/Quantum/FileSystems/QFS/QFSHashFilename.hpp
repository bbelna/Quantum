/**
 * @file Include/Quantum/FileSystems/QFS/QFSHashFilename.hpp
 * @brief Declares @ref QFSHashFilename.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::FileSystems::QFS {
  /**
   * @brief Computes FNV-1a 32-bit hash of a filename for directory lookup.
   * @note The hash is computed over the ASCII-lowercased bytes for
   *       case-insensitive matching.
   * @param name Pointer to the filename bytes (not necessarily
   *             null-terminated).
   * @param length Length of the filename in bytes.
   * @return The 32-bit FNV-1a hash.
   */
  inline constexpr UInt32 QFSHashFilename(
    const char* name,
    UInt32 length
  ) {
    UInt32 hash = 0x811C9DC5;  // FNV offset basis

    for (UInt32 i = 0; i < length; ++i) {
      UInt8 byte = static_cast<UInt8>(name[i]);

      // ASCII-range lowercase conversion
      if (byte >= 'A' && byte <= 'Z') {
        byte += 0x20;
      }

      hash ^= byte;
      hash *= 0x01000193;  // FNV prime
    }

    return hash;
  }
}
