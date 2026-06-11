/**
 * @file Include/Quantum/Kernel/Memory/MemoryTypes.hpp
 * @brief Declaration of kernel-wide memory types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>

namespace Quantum::Kernel::Memory {
  /**
   * @brief ID for a shared memory buffer.
   *
   * Returned by `Memory_CreateShared` and passed to `Memory_AttachShared` to
   * map the same blocks into another process' address space.
   */
  using SharedBufferID = UInt32;
}
