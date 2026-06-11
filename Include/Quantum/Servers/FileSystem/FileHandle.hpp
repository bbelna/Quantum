/**
 * @file Include/Quantum/Servers/FileSystem/FileHandle.hpp
 * @brief Declares @ref @QFSAbi::FileHandle.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Servers::FileSystem::ABI {
  /**
   * @brief Opaque handle to an open file. Zero indicates an invalid handle.
   */
  using FileHandle = UInt32;
}
