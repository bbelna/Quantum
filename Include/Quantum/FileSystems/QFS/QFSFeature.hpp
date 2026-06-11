/**
 * @file Include/Quantum/FileSystems/QFS/QFSFeature.hpp
 * @brief Declares @ref QFSFeature.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Enum.hpp>
#include <Quantum/Core/Types.hpp>

namespace Quantum::FileSystems::QFS {
  /**
   * @brief Feature flag bitmask for the
   *        @ref QFSSuperblock::FeatureFlags field.
   */
  enum class QFSFeature : UInt32 {
    /**
     * @brief Metadata journaling enabled.
     */
    Journal = 1 << 0,

    /**
     * @brief Extent-based file data allocation.
     */
    Extents = 1 << 1,

    /**
     * @brief B-tree directories.
     */
    DirectoryBTree = 1 << 2,

    /**
     * @brief Metadata checksums enabled.
     */
    MetadataChecksum = 1 << 3
  };
}

QUANTUM_ENABLE_ENUM_BITMASK_OPS(Quantum::FileSystems::QFS, QFSFeature)
