/**
 * @file Include/Quantum/Kernel/KernelOperationResult.hpp
 * @brief Declares @ref @QKrnl::KernelOperationResult.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>

namespace Quantum::Kernel {
  /**
   * @brief Result structure for kernel ABI operations that return multiple
   *        values.
   *
   * Captures the primary result and up to three secondary output arguments
   * from a kernel invocation, allowing callers to read back all return
   * values without resorting to inline assembly.
   */
  struct KernelOperationResult {
    /**
     * @brief Primary result.
     */
    UInt32 Result = 0;

    /**
     * @brief First output argument.
     */
    UInt32 Arg1 = 0;

    /**
     * @brief Second output argument.
     */
    UInt32 Arg2 = 0;

    /**
     * @brief Third output argument.
     */
    UInt32 Arg3 = 0;
  };
}
