/**
 * @file Include/Quantum/FileSystems/QFS/QFSJournalBlockTagFlag.hpp
 * @brief Declares @ref QFSJournalBlockTagFlag.
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
   * @brief QFS journal block tag flag bitmask.
   */
  enum class QFSJournalBlockTagFlag : UInt32 {
    /**
     * @brief Block data contains journal magic bytes and was escaped.
     */
    Escape = 1 << 0
  };
}

QUANTUM_ENABLE_ENUM_BITMASK_OPS(
  Quantum::FileSystems::QFS, QFSJournalBlockTagFlag
)
