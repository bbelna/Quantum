/**
 * @file Kernel/Handlers/IOutOfMemoryHandler.hpp
 * @brief Declares @ref @QKrnl::IOutOfMemoryHandler.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

namespace Quantum::Kernel::Handlers {
  /**
   * @brief Abstract interface for handling kernel out-of-memory conditions.
   *
   * Called by the kernel memory allocator when no free blocks can be found.
   * Implementations should dump diagnostic information (per-process page
   * counts, shared buffer stats) and optionally attempt to reclaim memory
   * (e.g. by killing the largest non-system process). If reclamation
   * succeeds, the allocator retries; otherwise it falls through to MAYDAY.
   */
  class IOutOfMemoryHandler {
    public:
      /**
       * @brief Destroys this @ref IOutOfMemoryHandler instance.
       */
      virtual ~IOutOfMemoryHandler() = default;

      /**
       * @brief Handles an out-of-memory condition.
       * @return `true` if memory was reclaimed and the allocator should
       *         retry the allocation; `false` if no recovery is possible.
       */
      virtual bool Handle() = 0;
  };
}
