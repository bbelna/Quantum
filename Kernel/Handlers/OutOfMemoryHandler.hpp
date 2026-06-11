/**
 * @file Kernel/OutOfMemoryHandler.hpp
 * @brief Declares @ref @QKrnl::OutOfMemoryHandler.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <KernelContext.hpp>

#include "IOutOfMemoryHandler.hpp"

namespace Quantum::Kernel::Handlers {
  /**
   * @brief @ref IOutOfMemoryHandler implementation that dumps per-process
   *        and shared buffer diagnostics before the kernel mayday.
   */
  class OutOfMemoryHandler : public IOutOfMemoryHandler {
    public:
      /**
       * @brief Creates a new @ref OutOfMemoryHandler.
       * @param context Pointer to the @ref KernelContext.
       */
      explicit OutOfMemoryHandler(KernelContext* context);

      /**
       * @brief Dumps out-of-memory diagnostics and returns `false`.
       * @return `false`, should proceed to `MAYDAY`.
       */
      bool Handle() override;

    private:
      /**
       * @brief Pointer to the @ref KernelContext.
       */
      KernelContext* _context = nullptr;

      /**
       * @brief Dumps user memory usage diagnostics.
       */
      void _dumpUserMemoryUsage();

      /**
       * @brief Dumps kernel memory usage diagnostics.
       */
      void _dumpKernelMemoryUsage();
  };
}
