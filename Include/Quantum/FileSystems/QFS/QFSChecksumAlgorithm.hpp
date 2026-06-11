/**
 * @file Include/Quantum/FileSystems/QFS/QFSChecksumAlgorithm.hpp
 * @brief Declares @ref QFSChecksumAlgorithm.
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
   * @brief Checksum algorithm identifiers.
   */
  enum class QFSChecksumAlgorithm : UInt32 {
    /**
     * @brief No checksums.
     */
    None = 0,

    /**
     * @brief CRC32 (ISO 3309 / ITU-T V.42, polynomial `0xEDB88320`).
     */
    CRC32 = 1
  };
}
