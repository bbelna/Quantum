/**
 * @file Include/Quantum/Kernel/KernelOperationPayload.hpp
 * @brief Declares @ref @QKrnl::KernelOperationPayload.
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
   * @brief Payload structure for direct kernel ABI operations invoked via
   *        @ref InvokeOS with subsystem 0.
   *
   * Packs up to three 32-bit arguments that map directly to the register
   * arguments of @ref Invoke (EBX, ESI, EDI on IA-32).
   */
  struct KernelOperationPayload {
    /**
     * @brief First argument (maps to arg1 / EBX).
     */
    UInt32 Arg1 = 0;

    /**
     * @brief Second argument (maps to arg2 / ESI).
     */
    UInt32 Arg2 = 0;

    /**
     * @brief Third argument (maps to arg3 / EDI).
     */
    UInt32 Arg3 = 0;
  };
}
